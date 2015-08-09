/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "sharp.h"
#define DEBUG 1

namespace sharp{

EWrapperImpl::EWrapperImpl()
	: m_pClient(new EPosixClientSocket(this))
	, m_state(ST_CONNECT)
	, m_sleepDeadline(0)
	, m_orderId(0)
	, host("")
	, port(7496)
	, clientId(0)
{
}

EWrapperImpl::~EWrapperImpl()
{
}

bool EWrapperImpl::connect(const char *host, unsigned int port, int clientId)
{
	// trying to connect
	printf( "Connecting to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, clientId);

	bool bRes = m_pClient->eConnect( host, port, clientId, /* extraAuth */ false);

	if (bRes) {
		printf( "Connected to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, clientId);
	}
	else
		printf( "Cannot connect to %s:%d clientId:%d\n", !( host && *host) ? "127.0.0.1" : host, port, clientId);

	return bRes;
}

void EWrapperImpl::disconnect() const
{
	m_pClient->eDisconnect();

	printf ( "Disconnected\n");
}

bool EWrapperImpl::isConnected() const
{
	return m_pClient->isConnected();
}

void EWrapperImpl::monitor()
{
	fd_set readSet, writeSet, errorSet;

	struct timeval tval;
	tval.tv_usec = 0;
	tval.tv_sec = 0;

	time_t now = time(NULL);

	if( m_sleepDeadline > 0) {
		// initialize timeout with m_sleepDeadline - now
		tval.tv_sec = m_sleepDeadline - now;
	}

	if( m_pClient->fd() >= 0 ) {

		FD_ZERO( &readSet);
		errorSet = writeSet = readSet;

		FD_SET( m_pClient->fd(), &readSet);

		if( !m_pClient->isOutBufferEmpty())
			FD_SET( m_pClient->fd(), &writeSet);

		FD_SET( m_pClient->fd(), &errorSet);

		int ret = select( m_pClient->fd() + 1, &readSet, &writeSet, &errorSet, &tval);

		if( ret == 0) { // timeout
			return;
		}

		if( ret < 0) {	// error
			disconnect();
			return;
		}

		if( m_pClient->fd() < 0)
			return;

		if( FD_ISSET( m_pClient->fd(), &errorSet)) {
			// error on socket
			m_pClient->onError();
		}

		if( m_pClient->fd() < 0)
			return;

		if( FD_ISSET( m_pClient->fd(), &writeSet)) {
			/**
			* socket is ready for writing
			* onSend() checks error by calling handleSocketError(), then calls
			* sendBufferedData(), sendBufferedData() calls EPosixClientSocket::send
			* (which calls ::send),  then sendBufferedData() clears m_outBuffer and returns
			* an int nResult.
			*/
			m_pClient->onSend();
		}

		if( m_pClient->fd() < 0)
			return;

		if( FD_ISSET( m_pClient->fd(), &readSet)) {
			/**
			* socket is ready for reading
			* onReceive() checks error by calling handleSocketError(), then calls
			* checkMessages(), checkMessages() first calls bufferedRead(), bufferedRead()
			* calls receive(which calls ::recv) and fills the m_inBuffer, then checkMessages()
			* calls processMsg() if m_connected is true,
			* ortherwise calls processConnectAck
			*/
			m_pClient->onReceive();
		}
	}
}

void EWrapperImpl::reqCurrentTime()
{
	printf( "Requesting Current Time\n");
	// set ping deadline to "now + n seconds"
	m_sleepDeadline = time( NULL) + PING_DEADLINE;
	m_pClient->reqCurrentTime();
}

void EWrapperImpl::placeOrder()
{
	auto & req = contract_order_request;
	req.orderId = m_orderId;
	printf( "Placing Order %ld: %s %ld %s at %f\n", req.orderId,
		req.order.action.c_str(), req.order.totalQuantity,
		req.contract.symbol.c_str(), req.order.lmtPrice);

	if(checkValidId(req.orderId)){
		m_pClient->placeOrder( req.orderId, req.contract, req.order);
	}else{
		assert(0 && "checkValidId failed !");
	}
	std::lock_guard<std::mutex> lk(mutex);
	placed_contract_orders.insert(req.orderId, req);
}

bool EWrapperImpl::cancelOrder(OrderId orderId)
{
	printf( "Cancelling Order %ld\n", orderId);

	auto & m = placed_contract_orders.orderId_index_map;
	if(m.count(orderId) == 1){
		m_pClient->cancelOrder(orderId);

		std::lock_guard<std::mutex> lk(mutex);
		auto & rds = placed_contract_orders.records;
		auto & resp = rds[m.at(orderId)]->response;
		assert(resp.orderId == orderId && "assert in EWrapperImpl::cancelOrder: resp.orderId == orderId failed.");
		resp.state = -1; // canceled
		return true;
	}else{
		std::cout<<"This orderId: "<<orderId<<" doesn't exist."<<std::endl;
		return false;
	}
}

// events called by m_pClient->onReceive();
void EWrapperImpl::orderStatus( OrderId orderId, const IBString &status, int filled,
	   int remaining, double avgFillPrice, int permId, int parentId,
	   double lastFillPrice, int clientId, const IBString& whyHeld)

{
    auto & m = placed_contract_orders.orderId_index_map;
	if(m.count(orderId) == 0){
		std::cout<<"This orderId:"<<orderId<<" hasn't been placed, why I am getting this message?"<<std::endl;
		return;
	}

	std::lock_guard<std::mutex> lk(mutex);
	auto & rds = placed_contract_orders.records;
	auto & resp = rds[m.at(orderId)]->response;
	assert(resp.orderId == orderId && "assert in EWrapperImpl::orderStatus: resp.orderId == orderId failed.");
	resp.state += 1;
	resp.clientId = clientId;
	resp.permId = permId;
	resp.parentId = parentId;
	resp.filled = filled;
	resp.remaining = remaining;
	resp.avgFillPrice = avgFillPrice;
	resp.lastFillPrice = lastFillPrice;
	resp.status = status;
	resp.whyHeld = whyHeld;

	printf( "Order: id=%ld, status=%s\n", orderId, status.c_str());

}

void EWrapperImpl::nextValidId( OrderId orderId)
{
	m_orderId = orderId;
	order_ids.push_back(orderId);
}

void EWrapperImpl::currentTime( long time)
{

	time_t t = ( time_t)time;
	struct tm * timeinfo = localtime ( &t);
	printf( "The current date/time is: %s", asctime( timeinfo));

	time_t now = ::time(NULL);
	m_sleepDeadline = now + SLEEP_BETWEEN_PINGS;
}

void EWrapperImpl::error(const int id, const int errorCode, const IBString errorString)
{
//	printf( "Error id=%d, errorCode=%d, msg=%s\n", id, errorCode, errorString.c_str());

	if( id == -1 && errorCode == 1100) // if "Connectivity between IB and TWS has been lost"
		disconnect();
}

void EWrapperImpl::tickPrice( TickerId tickerId, TickType field, double price, int canAutoExecute) {}
void EWrapperImpl::tickSize( TickerId tickerId, TickType field, int size) {}
void EWrapperImpl::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
											 double optPrice, double pvDividend,
											 double gamma, double vega, double theta, double undPrice) {}
void EWrapperImpl::tickGeneric(TickerId tickerId, TickType tickType, double value) {}
void EWrapperImpl::tickString(TickerId tickerId, TickType tickType, const IBString& value) {}
void EWrapperImpl::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
							   double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry) {}
void EWrapperImpl::openOrder( OrderId orderId, const Contract&, const Order&, const OrderState& ostate) {}
void EWrapperImpl::openOrderEnd() {}
void EWrapperImpl::winError( const IBString &str, int lastError) {}
void EWrapperImpl::connectionClosed() {}
void EWrapperImpl::updateAccountValue(const IBString& key, const IBString& val,
										  const IBString& currency, const IBString& accountName) {}
void EWrapperImpl::updatePortfolio(const Contract& contract, int position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL, const IBString& accountName){}
void EWrapperImpl::updateAccountTime(const IBString& timeStamp) {}
void EWrapperImpl::accountDownloadEnd(const IBString& accountName) {}
void EWrapperImpl::contractDetails( int reqId, const ContractDetails& contractDetails) {}
void EWrapperImpl::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}
void EWrapperImpl::contractDetailsEnd( int reqId) {}
void EWrapperImpl::execDetails( int reqId, const Contract& contract, const Execution& execution) {}
void EWrapperImpl::execDetailsEnd( int reqId) {}

void EWrapperImpl::updateMktDepth(TickerId id, int position, int operation, int side,
									  double price, int size) {}
void EWrapperImpl::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
										int side, double price, int size) {}
void EWrapperImpl::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {}
void EWrapperImpl::managedAccounts( const IBString& accountsList) {}
void EWrapperImpl::receiveFA(faDataType pFaDataType, const IBString& cxml) {}
void EWrapperImpl::historicalData(TickerId reqId, const IBString& date, double open, double high,
									  double low, double close, int volume, int barCount, double WAP, int hasGaps) {}
void EWrapperImpl::scannerParameters(const IBString &xml) {}
void EWrapperImpl::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
	   const IBString &distance, const IBString &benchmark, const IBString &projection,
	   const IBString &legsStr) {}
void EWrapperImpl::scannerDataEnd(int reqId) {}
void EWrapperImpl::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
								   long volume, double wap, int count) {}
void EWrapperImpl::fundamentalData(TickerId reqId, const IBString& data) {}
void EWrapperImpl::deltaNeutralValidation(int reqId, const UnderComp& underComp) {}
void EWrapperImpl::tickSnapshotEnd(int reqId) {}
void EWrapperImpl::marketDataType(TickerId reqId, int marketDataType) {}
void EWrapperImpl::commissionReport( const CommissionReport& commissionReport) {}
void EWrapperImpl::position( const IBString& account, const Contract& contract, int position, double avgCost) {}
void EWrapperImpl::positionEnd() {}
void EWrapperImpl::accountSummary( int reqId, const IBString& account, const IBString& tag, const IBString& value, const IBString& curency) {}
void EWrapperImpl::accountSummaryEnd( int reqId) {}
void EWrapperImpl::verifyMessageAPI( const IBString& apiData) {}
void EWrapperImpl::verifyCompleted( bool isSuccessful, const IBString& errorText) {}
void EWrapperImpl::displayGroupList( int reqId, const IBString& groups) {}
void EWrapperImpl::displayGroupUpdated( int reqId, const IBString& contractInfo) {}

bool EWrapperImpl::checkValidId( OrderId orderId){
	int count = std::count(order_ids.begin(), order_ids.end(), orderId);
	if( count == 1 )return true;
	else return false;
}



} // end of namespace sharp
