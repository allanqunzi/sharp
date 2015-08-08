/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "PosixTestClient.h"
#include "EPosixClientSocket.h"
#include "EPosixClientSocketPlatform.h"
#define DEBUG 1

const int PING_DEADLINE = 2; // seconds
const int SLEEP_BETWEEN_PINGS = 30; // seconds

/* the following is by Aiqun Huang */


///////////////////////////////////////////////////////////
// member funcs
PosixTestClient::PosixTestClient()
	: m_pClient(new EPosixClientSocket(this))
	, m_state(ST_CONNECT)
	, m_sleepDeadline(0)
	, m_orderId(0)
	, host("")
	, port(7496)
	, clientId(0)
{
}

PosixTestClient::~PosixTestClient()
{
}

bool PosixTestClient::connect(const char *host, unsigned int port, int clientId)
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

void PosixTestClient::disconnect() const
{
	m_pClient->eDisconnect();

	printf ( "Disconnected\n");
}

bool PosixTestClient::isConnected() const
{
	return m_pClient->isConnected();
}

void PosixTestClient::monitor()
{
	fd_set readSet, writeSet, errorSet;

	struct timeval tval;
	tval.tv_usec = 0;
	tval.tv_sec = 0;

	time_t now = time(NULL);
/*
	switch (m_state) {
		case ST_PLACEORDER:
			//placeOrder();
			break;
		case ST_PLACEORDER_ACK:
			break;
		case ST_CANCELORDER:
			//cancelOrder();
			break;
		case ST_CANCELORDER_ACK:
			break;
		case ST_PING:
			reqCurrentTime();
			break;
		case ST_PING_ACK:
			if( m_sleepDeadline < now) {
				disconnect();
				return;
			}
			break;
		case ST_IDLE:
			if( m_sleepDeadline < now) {
				m_state = ST_PING;
				return;
			}
			break;
	}
*/
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
			// socket is ready for writing
			// onSend() checks error by calling handleSocketError(), then calls
			// sendBufferedData(), sendBufferedData() calls EPosixClientSocket::send
			// (which calls ::send),  then sendBufferedData() clears m_outBuffer and returns
			// an int nResult.
			m_pClient->onSend();
		}

		if( m_pClient->fd() < 0)
			return;

		if( FD_ISSET( m_pClient->fd(), &readSet)) {
			// socket is ready for reading
			// onReceive() checks error by calling handleSocketError(), then calls
			// checkMessages(), checkMessages() first calls bufferedRead(), bufferedRead()
			// calls receive(which calls ::recv) and fills the m_inBuffer, then checkMessages()
			// calls processMsg() if m_connected is true,
			// ortherwise calls processConnectAck
			m_pClient->onReceive();
		}
	}
}

//////////////////////////////////////////////////////////////////
// methods
void PosixTestClient::reqCurrentTime()
{
	printf( "Requesting Current Time\n");

	// set ping deadline to "now + n seconds"
	m_sleepDeadline = time( NULL) + PING_DEADLINE;

	m_state = ST_PING_ACK;

	m_pClient->reqCurrentTime();
}

void PosixTestClient::placeOrder(ContractOrder & contract_order)
{
	/***
	Contract contract;
	Order order;

	contract.symbol = "IBM";
	contract.secType = "STK";
	contract.exchange = "SMART";
	contract.currency = "USD";

	order.action = "BUY";
	order.totalQuantity = 1000;
	order.orderType = "LMT";
	order.lmtPrice = 0.01;
	*/

	printf( "Placing Order %ld: %s %ld %s at %f\n", m_orderId,
		contract_order.order.action.c_str(), contract_order.order.totalQuantity,
		contract_order.contract.symbol.c_str(), contract_order.order.lmtPrice);

	m_state = ST_PLACEORDER_ACK;

	contract_order.orderId = m_orderId;
	if(checkValidId(contract_order.orderId)){
		m_pClient->placeOrder( contract_order.orderId, contract_order.contract, contract_order.order);
	}else{
		assert(0 && "checkValidId failed !");
	}
	std::lock_guard<std::mutex> lk(mutex);
	auto it = FindorCreate<OrderId, OrderResponse>(contract_order.orderId, order_statuses);
	auto & sec = it->second;
	sec->state = 0; // order submitted
	placed_contract_orders.update(contract_order.orderId, contract_order);
}

bool PosixTestClient::cancelOrder(OrderId orderId)
{
	printf( "Cancelling Order %ld\n", orderId);

	m_state = ST_CANCELORDER_ACK;

	if(order_statuses.count(orderId) == 1){
		m_pClient->cancelOrder(orderId);
		auto & ptr =  order_statuses[orderId];
		ptr->state = -1; // canceled
		return true;
	}else{
		std::cout<<"This orderId: "<<orderId<<" doesn't exist."<<std::endl;
		return false;
	}
}
///////////////////////////////////////////////////////////////////
// events
void PosixTestClient::orderStatus( OrderId orderId, const IBString &status, int filled,
	   int remaining, double avgFillPrice, int permId, int parentId,
	   double lastFillPrice, int clientId, const IBString& whyHeld)

{
	{   // written by Aiqun Huang
		if(order_statuses.count(orderId) == 0){
			std::cout<<"This orderId:"<<orderId<<" hasn't been placed, why I am getting this message?"<<std::endl;
			return;
		}
		std::lock_guard<std::mutex> lk(mutex);
		auto & ptr = order_statuses.at(orderId);
		ptr->orderId = orderId;
		ptr->state += 1;
		ptr->status = status;
		ptr->filled = filled;
		ptr->remaining = remaining;
		ptr->avgFillPrice = avgFillPrice;
		ptr->permId = permId;
		ptr->parentId = parentId;
		ptr->lastFillPrice = lastFillPrice;
		ptr->clientId = clientId;
		ptr->whyHeld = whyHeld;
    }

	if( orderId == m_orderId) {

		if( m_state == ST_PLACEORDER_ACK && (status == "PreSubmitted" || status == "Submitted"))
			m_state = ST_CANCELORDER;

		if( m_state == ST_CANCELORDER_ACK && status == "Cancelled")
			m_state = ST_PING;

		printf( "Order: id=%ld, status=%s\n", orderId, status.c_str());
	}
}



void PosixTestClient::nextValidId( OrderId orderId)
{
	m_orderId = orderId;
	order_ids.push_back(orderId);

	m_state = ST_PLACEORDER;
}

void PosixTestClient::currentTime( long time)
{
	if ( m_state == ST_PING_ACK) {
		time_t t = ( time_t)time;
		struct tm * timeinfo = localtime ( &t);
		printf( "The current date/time is: %s", asctime( timeinfo));

		time_t now = ::time(NULL);
		m_sleepDeadline = now + SLEEP_BETWEEN_PINGS;

		m_state = ST_IDLE;
	}
}

void PosixTestClient::error(const int id, const int errorCode, const IBString errorString)
{
//	printf( "Error id=%d, errorCode=%d, msg=%s\n", id, errorCode, errorString.c_str());

	if( id == -1 && errorCode == 1100) // if "Connectivity between IB and TWS has been lost"
		disconnect();
}

void PosixTestClient::tickPrice( TickerId tickerId, TickType field, double price, int canAutoExecute) {}
void PosixTestClient::tickSize( TickerId tickerId, TickType field, int size) {}
void PosixTestClient::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
											 double optPrice, double pvDividend,
											 double gamma, double vega, double theta, double undPrice) {}
void PosixTestClient::tickGeneric(TickerId tickerId, TickType tickType, double value) {}
void PosixTestClient::tickString(TickerId tickerId, TickType tickType, const IBString& value) {}
void PosixTestClient::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
							   double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry) {}
void PosixTestClient::openOrder( OrderId orderId, const Contract&, const Order&, const OrderState& ostate) {}
void PosixTestClient::openOrderEnd() {}
void PosixTestClient::winError( const IBString &str, int lastError) {}
void PosixTestClient::connectionClosed() {}
void PosixTestClient::updateAccountValue(const IBString& key, const IBString& val,
										  const IBString& currency, const IBString& accountName) {}
void PosixTestClient::updatePortfolio(const Contract& contract, int position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL, const IBString& accountName){}
void PosixTestClient::updateAccountTime(const IBString& timeStamp) {}
void PosixTestClient::accountDownloadEnd(const IBString& accountName) {}
void PosixTestClient::contractDetails( int reqId, const ContractDetails& contractDetails) {}
void PosixTestClient::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}
void PosixTestClient::contractDetailsEnd( int reqId) {}
void PosixTestClient::execDetails( int reqId, const Contract& contract, const Execution& execution) {}
void PosixTestClient::execDetailsEnd( int reqId) {}

void PosixTestClient::updateMktDepth(TickerId id, int position, int operation, int side,
									  double price, int size) {}
void PosixTestClient::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
										int side, double price, int size) {}
void PosixTestClient::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {}
void PosixTestClient::managedAccounts( const IBString& accountsList) {}
void PosixTestClient::receiveFA(faDataType pFaDataType, const IBString& cxml) {}
void PosixTestClient::historicalData(TickerId reqId, const IBString& date, double open, double high,
									  double low, double close, int volume, int barCount, double WAP, int hasGaps) {}
void PosixTestClient::scannerParameters(const IBString &xml) {}
void PosixTestClient::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
	   const IBString &distance, const IBString &benchmark, const IBString &projection,
	   const IBString &legsStr) {}
void PosixTestClient::scannerDataEnd(int reqId) {}
void PosixTestClient::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
								   long volume, double wap, int count) {}
void PosixTestClient::fundamentalData(TickerId reqId, const IBString& data) {}
void PosixTestClient::deltaNeutralValidation(int reqId, const UnderComp& underComp) {}
void PosixTestClient::tickSnapshotEnd(int reqId) {}
void PosixTestClient::marketDataType(TickerId reqId, int marketDataType) {}
void PosixTestClient::commissionReport( const CommissionReport& commissionReport) {}
void PosixTestClient::position( const IBString& account, const Contract& contract, int position, double avgCost) {}
void PosixTestClient::positionEnd() {}
void PosixTestClient::accountSummary( int reqId, const IBString& account, const IBString& tag, const IBString& value, const IBString& curency) {}
void PosixTestClient::accountSummaryEnd( int reqId) {}
void PosixTestClient::verifyMessageAPI( const IBString& apiData) {}
void PosixTestClient::verifyCompleted( bool isSuccessful, const IBString& errorText) {}
void PosixTestClient::displayGroupList( int reqId, const IBString& groups) {}
void PosixTestClient::displayGroupUpdated( int reqId, const IBString& contractInfo) {}



//  the following code is defined by Aiqun Huang

bool PosixTestClient::checkValidId( OrderId orderId){
	int count = std::count(order_ids.begin(), order_ids.end(), orderId);
	if( count == 1 )return true;
	else return false;
}




