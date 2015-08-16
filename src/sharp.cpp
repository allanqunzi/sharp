/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "sharp.h"
#define DEBUG 1

namespace sharp{

EWrapperImpl::EWrapperImpl( const std::vector<std::string> wl )
	: m_pClient(new EPosixClientSocket(this))
	, m_state(ST_CONNECT)
	, m_sleepDeadline(0)
	, m_orderId(0)
	, host("")
	, port(7496)
	, clientId(0)
	, watch_list(DEFAULT_BUCKETS_NUM)
	, watch_list_bars(DEFAULT_BUCKETS_NUM)
	, bar_mutexes(DEFAULT_BUCKETS_NUM)
{
	for(auto e : wl){
		watch_list[e] = ticker_id.getNewId();
		bar_mutexes[watch_list[e]];
	}
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

	LOG(info)<<"Disconnected";
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

void EWrapperImpl::reqCurrentTime()
{
	LOG(info)<<"Requesting Current Time";
	// set ping deadline to "now + n seconds"
	m_sleepDeadline = time( NULL) + PING_DEADLINE;
	m_pClient->reqCurrentTime();
}

void EWrapperImpl::placeOrder()
{
	auto & req = contract_order_request;
	req.orderId = order_id.getNewId();
	used_order_ids.push_back(req.orderId);

	printf( "Placing Order %ld: %s %ld %s at %f\n", req.orderId,
		req.order.action.c_str(), req.order.totalQuantity,
		req.contract.symbol.c_str(), req.order.lmtPrice);

	m_pClient->placeOrder( req.orderId, req.contract, req.order);

	std::lock_guard<std::mutex> lk(mutex);
	req.response.orderId = req.orderId;
	req.response.state = 0;
	placed_contract_orders.insert(req.orderId, req);
}

bool EWrapperImpl::cancelOrder(OrderId orderId)
{
	LOG(info)<<"Cancelling Order "<<orderId;

	auto & m = placed_contract_orders.orderId_index_map;
	if(m.count(orderId) == 1){
		m_pClient->cancelOrder(orderId);
		std::lock_guard<std::mutex> lk(mutex);
		auto & rds = placed_contract_orders.records;
		auto & resp = rds[m.at(orderId)]->response;
		assert(resp.orderId == orderId && "error: assert in EWrapperImpl::cancelOrder: resp.orderId == orderId failed.");
		resp.state = -1; // canceled
		return true;
	}else{
		LOG(warning)<<"This orderId: "<<orderId<<" doesn't exist.";
		return false;
	}
}

// event called by m_pClient->onReceive();
void EWrapperImpl::orderStatus( OrderId orderId, const IBString &status, int filled,
	   int remaining, double avgFillPrice, int permId, int parentId,
	   double lastFillPrice, int clientId, const IBString& whyHeld)

{
	std::lock_guard<std::mutex> lk(mutex);
	auto & m = placed_contract_orders.orderId_index_map;
	auto & rds = placed_contract_orders.records;
	if(m.count(orderId) == 0){
		LOG(info)<<"get status messages for previously placed order, orderId = "<<orderId;
		ContractOrder co;
		co.orderId = orderId;
		co.response.orderId = orderId;
		placed_contract_orders.insert(orderId, co);
	}
	auto & resp = rds[m.at(orderId)]->response;
	assert(resp.orderId == orderId && "error: assert in EWrapperImpl::orderStatus: resp.orderId == orderId failed.");
	if(resp.state != -1){ resp.state += 1; } // -1 means this order is previously placed
	resp.clientId = clientId;
	resp.permId = permId;
	resp.parentId = parentId;
	resp.filled = filled;
	resp.remaining = remaining;
	resp.avgFillPrice = avgFillPrice;
	resp.lastFillPrice = lastFillPrice;
	resp.status = status;
	resp.whyHeld = whyHeld;

	LOG(info)<<"Order: id = "<<orderId<<", status = "<<status;
}

void EWrapperImpl::reqOpenOrders(){
	open_order_set.clear();
	open_order_flag.store(false, std::memory_order_relaxed);
	m_pClient->reqOpenOrders();
}

void EWrapperImpl::reqAllOpenOrders(){
	open_order_set.clear();
	open_order_flag.store(false, std::memory_order_relaxed);
	m_pClient->reqAllOpenOrders();
}
bool EWrapperImpl::reqGlobalCancel(){
	try{
		m_pClient->reqGlobalCancel();
		return true;
	}catch(...){
		return false;
	}
}

// event called by m_pClient->onReceive();
// this function and orderStatus are never called at the same time, a lock is unnecessary.
void EWrapperImpl::openOrder( OrderId orderId, const Contract& c, const Order& o, const OrderState& ostate) {
	open_order_set.insert(orderId);
	auto & m = placed_contract_orders.orderId_index_map;
	if(m.count(orderId) == 0){
		LOG(info)<<"get status messages for previously placed order, still open, orderId = "<<orderId;
		ContractOrder co;
		co.orderId = orderId;
		co.contract = c;
		co.order = o;
		co.response.orderId = orderId;
		co.response.status = ostate.status;
		placed_contract_orders.insert(orderId, co);
	}
}

// event called by m_pClient->onReceive();
void EWrapperImpl::openOrderEnd() {
	open_order_flag.store(true, std::memory_order_relaxed);
}

// event called by m_pClient->onReceive();
void EWrapperImpl::nextValidId( OrderId orderId)
{
	m_orderId = orderId;
	order_id.setInitial(orderId - 1L);
	LOG(info)<<"m_orderId updated, order_id.id is initialized to "<<(orderId - 1);
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
	// LOG(error)<<"Error id = "<<id<<", errorCode = "<<errorCode<<", msg = "<<errorString;
	if( id == -1 && errorCode == 1100) // if "Connectivity between IB and TWS has been lost"
		disconnect();
}

void EWrapperImpl::reqMarketSnapshot()
{
	Contract contract;
	contract.symbol = "AMZN";
	contract.secType = "STK";
	contract.exchange = "SMART";
	contract.primaryExchange = "NYSE";
	contract.strike = 0.0;
	contract.includeExpired =false;
	contract.currency = "USD";

	IBString genericTicks = "";
	TagValueListSPtr mktDataOptions, chartOptions, realTimeBarsOptions, scannerSubscriptionOptions;
	IBString endDateTime = "20150804 10:10:45";
	IBString durationStr = "2 D";
	IBString barSizeSetting = "1 min";
	IBString whatToShow = "TRADES";
	int useRTH = 0;
	int formatDate = 1;
/*
    m_pClient->reqHistoricalData( 0, contract, endDateTime, durationStr, barSizeSetting, whatToShow,
    	useRTH, formatDate, chartOptions);

    m_pClient->reqMarketDataType(2);
    // calling reqMktData returns noisy ticks
    m_pClient->reqMktData(0, contract, genericTicks, true, mktDataOptions);

    m_pClient->reqRealTimeBars(0, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "AAPL";
    m_pClient->reqRealTimeBars(1, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "BABA";
    m_pClient->reqRealTimeBars(2, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "NFLX";
    m_pClient->reqRealTimeBars(3, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "GOOG";
    m_pClient->reqRealTimeBars(4, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "BIDU";
    m_pClient->reqRealTimeBars(5, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "GS";
    m_pClient->reqRealTimeBars(6, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "JPM";
    m_pClient->reqRealTimeBars(7, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "MS";
    m_pClient->reqRealTimeBars(8, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "NDRO";
    m_pClient->reqRealTimeBars(9, contract, 5, whatToShow, false, realTimeBarsOptions);
    contract.symbol = "DOM";
    m_pClient->reqRealTimeBars(10, contract, 5, whatToShow, false, realTimeBarsOptions);
*/
    // m_pClient->reqScannerParameters();

    ScannerSubscription scanner;
    scanner.instrument = "STK";
    scanner.locationCode = "STK.US.MAJOR";
    scanner.abovePrice = 10.00;
    scanner.scanCode = "TOP_PERC_GAIN";
    // m_pClient->reqScannerSubscription(0, scanner, scannerSubscriptionOptions);

/*
    contract.symbol = "AMZN";
    contract.secType = "OPT";
    contract.strike = 540.00;
    contract.right = "CALL";
    contract.expiry = "201509";
    contract.currency = "USD";
*/
    contract.conId = 202465243;
    contract.exchange = "SMART";
    // calling reqContractDetails can get all the info for an option chain
    // m_pClient->reqContractDetails(0, contract);
    // still need to check and make sure with IB on if they provide market data for options
    // m_pClient->reqMktData(0, contract, genericTicks, true, mktDataOptions);
    // m_pClient->reqRealTimeBars(10, contract, 5, whatToShow, false, realTimeBarsOptions);
}

void EWrapperImpl::tickPrice( TickerId tickerId, TickType field, double price, int canAutoExecute) {
	LOG(info)<<"calling EWrapperImpl::tickPrice";
}

void EWrapperImpl::tickSize( TickerId tickerId, TickType field, int size) {
	LOG(info)<<"calling EWrapperImpl::tickSize";
}

void EWrapperImpl::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
											double optPrice, double pvDividend,
											double gamma, double vega, double theta, double undPrice)
{
	LOG(info)<<"calling EWrapperImpl::tickOptionComputation";
}

void EWrapperImpl::tickGeneric(TickerId tickerId, TickType tickType, double value) {
	LOG(info)<<"calling EWrapperImpl::tickGeneric";
}

void EWrapperImpl::tickString(TickerId tickerId, TickType tickType, const IBString& value) {
	LOG(info)<<"calling EWrapperImpl::tickString";
}

void EWrapperImpl::tickEFP(TickerId tickerId, TickType tickType, double basisPoints,
							const IBString& formattedBasisPoints,
							double totalDividends, int holdDays, const IBString& futureExpiry,
							double dividendImpact, double dividendsToExpiry)
{
	LOG(info)<<"calling EWrapperImpl::tickEFP";
}

void EWrapperImpl::tickSnapshotEnd(int reqId) {
	LOG(info)<<"calling EWrapperImpl::tickSnapshotEnd";
}

void EWrapperImpl::historicalData(TickerId reqId, const IBString& date, double open, double high,
									double low, double close, int volume, int barCount,
									double WAP, int hasGaps)
{
	LOG(info)<<"calling EWrapperImpl::historicalData";
}

void EWrapperImpl::realtimeBar(TickerId reqId, long time, double open, double high, double low,
								double close, long volume, double wap, int count)
{
	LOG(info)<<"EWrapperImpl::realtimeBar";
	try{
		auto & bars = watch_list_bars.at(reqId);
		{
			std::lock_guard<std::mutex> lk(bar_mutexes.at(reqId));
			bars.push(RealTimeBar(reqId, time, open, low, high, close, volume, wap, count));
		}
		bars.cv.notify_one();
	}catch(const std::out_of_range& oor){
		LOG(error)<<"I haven't requested yet, why I am getting bars for reqId = "<<reqId;
	}
}

void EWrapperImpl::scannerParameters(const IBString &xml) {
	LOG(info)<<"calling EWrapperImpl::scannerParameters";
}

void EWrapperImpl::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
	   const IBString &distance, const IBString &benchmark, const IBString &projection,
	   const IBString &legsStr) {
	LOG(info)<<"calling EWrapperImpl::scannerData";
}
void EWrapperImpl::scannerDataEnd(int reqId) {
	LOG(info)<<"calling EWrapperImpl::scannerDataEnd";
}

void EWrapperImpl::contractDetails( int reqId, const ContractDetails& contractDetails) {
	LOG(info)<<"calling EWrapperImpl::contractDetails";
	std::cout<<contractDetails.category<<std::endl;
	std::cout<<contractDetails.contractMonth<<std::endl;
	std::cout<<contractDetails.industry<<std::endl;
	std::cout<<contractDetails.liquidHours<<std::endl;
	std::cout<<contractDetails.longName<<std::endl;
	std::cout<<contractDetails.marketName<<std::endl;
	std::cout<<contractDetails.minTick<<std::endl;
	std::cout<<contractDetails.orderTypes<<std::endl;
	std::cout<<contractDetails.priceMagnifier<<std::endl;
	std::cout<<contractDetails.subcategory<<std::endl;
	std::cout<<"contract option"<<contractDetails.summary.right<<std::endl;
	std::cout<<"contract option"<<contractDetails.summary.strike<<std::endl;
	std::cout<<"contract option"<<contractDetails.summary.conId<<std::endl;
	std::cout<<"contract option"<<contractDetails.summary.exchange<<std::endl;
	std::cout<<contractDetails.underConId<<std::endl;
	std::cout<<contractDetails.validExchanges<<std::endl;
}

void EWrapperImpl::contractDetailsEnd( int reqId) {
	LOG(info)<<"calling EWrapperImpl::contractDetailsEnd";
}


void EWrapperImpl::winError( const IBString &str, int lastError) {}
void EWrapperImpl::connectionClosed() {}
void EWrapperImpl::updateAccountValue(const IBString& key, const IBString& val,
										  const IBString& currency, const IBString& accountName) {}
void EWrapperImpl::updatePortfolio(const Contract& contract, int position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL, const IBString& accountName){}
void EWrapperImpl::updateAccountTime(const IBString& timeStamp) {}
void EWrapperImpl::accountDownloadEnd(const IBString& accountName) {}

void EWrapperImpl::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}

void EWrapperImpl::execDetails( int reqId, const Contract& contract, const Execution& execution) {}
void EWrapperImpl::execDetailsEnd( int reqId) {}

void EWrapperImpl::updateMktDepth(TickerId id, int position, int operation, int side,
									  double price, int size) {}
void EWrapperImpl::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
										int side, double price, int size) {}
void EWrapperImpl::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {}
void EWrapperImpl::managedAccounts( const IBString& accountsList) {}
void EWrapperImpl::receiveFA(faDataType pFaDataType, const IBString& cxml) {}

void EWrapperImpl::fundamentalData(TickerId reqId, const IBString& data) {}
void EWrapperImpl::deltaNeutralValidation(int reqId, const UnderComp& underComp) {}

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
	int count = std::count(used_order_ids.begin(), used_order_ids.end(), orderId);
	if( count == 1 )return true;
	else return false;
}

bool EWrapperImpl::addToWatchList( const std::vector<std::string> & wl){
	Contract contract;
	contract.secType = "STK";
	contract.exchange = "SMART";
	contract.primaryExchange = "NYSE";
	contract.currency = "USD";
	TagValueListSPtr realTimeBarsOptions;
	IBString whatToShow = "TRADES";

	for(auto & e : wl){
		if(watch_list.count(e) == 0){
			auto id = watch_list[e] = ticker_id.getNewId();
			LOG(info)<<"adding "<<e<<" to watch_list, reqId = "<<id;
			// no need to put a lock here, the key is id (a number)
			bar_mutexes[id];
			watch_list_bars[id];
			contract.symbol = e;
			// true: only trading hours data
			m_pClient->reqRealTimeBars(id, contract, 5, whatToShow, false, realTimeBarsOptions);
		}
	}
	return true;
}

bool EWrapperImpl::removeFromWatchList( const std::vector<std::string> & rm){
	for(auto & e : rm){
		auto it = watch_list.find(e);
		if(it != watch_list.end()){
			LOG(info)<<"removing "<<e<<" from watch_list";
			m_pClient->cancelRealTimeBars(it->second);
			// not sure when ib tws stops calling EWrapperImpl::realtimeBar
			// the safest way is not to erase the corresponding watch_list_bars and
			// bar_mutexes, this is a not big issue unless there is a huge number of
			// unerased ones, which happens very rarely.
			// std::this_thread::sleep_for(2 * BAR_WAITING_TIME);
			// watch_list_bars.erase(it->second);
			// bar_mutexes.erase(it->second);
			watch_list.erase(it);
		}
	}
	return true;
}

bool EWrapperImpl::removeZombieSymbols(const std::vector<std::string> & wl){
	std::vector<std::string> rm;
	for( auto & e : watch_list){
		if ( std::find(wl.begin(), wl.end(), e.first) == wl.end() ){
			rm.push_back(e.first);
		}
	}
	return removeFromWatchList(rm);
}

// this function is rarely used
bool EWrapperImpl::requestRealTimeBars(){
	Contract contract;
	contract.secType = "STK";
	contract.exchange = "SMART";
	contract.primaryExchange = "NYSE";
	contract.currency = "USD";
	TagValueListSPtr realTimeBarsOptions;
	IBString whatToShow = "TRADES";

	for(auto & m : watch_list){
		contract.symbol = m.first;
		m_pClient->reqRealTimeBars(m.second, contract, 5, whatToShow, true, realTimeBarsOptions);
	}

	return true;
}

std::string EWrapperImpl::getField(TickType tickType){
	switch (tickType)
	{
		case BID_SIZE:	                    return "bidSize";
		case BID:                           return "bidPrice";
		case ASK:                           return "askPrice";
		case ASK_SIZE:	                    return "askSize";
		case LAST:                          return "lastPrice";
		case LAST_SIZE:	                    return "lastSize";
		case HIGH:                          return "high";
		case LOW:                           return "low";
		case VOLUME:	                    return "volume";
		case CLOSE:                         return "close";
		case BID_OPTION_COMPUTATION:        return "bidOptComp";
		case ASK_OPTION_COMPUTATION:        return "askOptComp";
		case LAST_OPTION_COMPUTATION:       return "lastOptComp";
		case MODEL_OPTION:                  return "optionModel";
		case OPEN:                          return "open";
		case LOW_13_WEEK:                   return "13WeekLow";
		case HIGH_13_WEEK:                  return "13WeekHigh";
		case LOW_26_WEEK:                   return "26WeekLow";
		case HIGH_26_WEEK:                  return "26WeekHigh";
		case LOW_52_WEEK:                   return "52WeekLow";
		case HIGH_52_WEEK:                  return "52WeekHigh";
		case AVG_VOLUME:                    return "AvgVolume";
		case OPEN_INTEREST:                 return "OpenInterest";
		case OPTION_HISTORICAL_VOL:         return "OptionHistoricalVolatility";
		case OPTION_IMPLIED_VOL:            return "OptionImpliedVolatility";
		case OPTION_BID_EXCH:               return "OptionBidExchStr";
		case OPTION_ASK_EXCH:               return "OptionAskExchStr";
		case OPTION_CALL_OPEN_INTEREST:     return "OptionCallOpenInterest";
		case OPTION_PUT_OPEN_INTEREST:      return "OptionPutOpenInterest";
		case OPTION_CALL_VOLUME:            return "OptionCallVolume";
		case OPTION_PUT_VOLUME:             return "OptionPutVolume";
		case INDEX_FUTURE_PREMIUM:          return "IndexFuturePremium";
		case BID_EXCH:                      return "bidExch";
		case ASK_EXCH:                      return "askExch";
		case AUCTION_VOLUME:                return "auctionVolume";
		case AUCTION_PRICE:                 return "auctionPrice";
		case AUCTION_IMBALANCE:             return "auctionImbalance";
		case MARK_PRICE:                    return "markPrice";
		case BID_EFP_COMPUTATION:           return "bidEFP";
		case ASK_EFP_COMPUTATION:           return "askEFP";
		case LAST_EFP_COMPUTATION:          return "lastEFP";
		case OPEN_EFP_COMPUTATION:          return "openEFP";
		case HIGH_EFP_COMPUTATION:          return "highEFP";
		case LOW_EFP_COMPUTATION:           return "lowEFP";
		case CLOSE_EFP_COMPUTATION:         return "closeEFP";
		case LAST_TIMESTAMP:                return "lastTimestamp";
		case SHORTABLE:                     return "shortable";
		case FUNDAMENTAL_RATIOS:            return "fundamentals";
		case RT_VOLUME:                     return "RTVolume";
		case HALTED:                        return "halted";
		case BID_YIELD:                     return "bidYield";
		case ASK_YIELD:                     return "askYield";
		case LAST_YIELD:                    return "lastYield";
		case CUST_OPTION_COMPUTATION:       return "custOptComp";
		case TRADE_COUNT:                   return "trades";
		case TRADE_RATE:                    return "trades/min";
		case VOLUME_RATE:                   return "volume/min";
		case LAST_RTH_TRADE:                return "lastRTHTrade";
		default:                            return "unknown";
	}
}



} // end of namespace sharp
