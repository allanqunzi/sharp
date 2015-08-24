/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#ifndef QUNZI_SHARP_HEADER
#define QUNZI_SHARP_HEADER

#include "CommonDefs.h"
#include "CommissionReport.h"
#include "Contract.h"
#include "Execution.h"
#include "OrderState.h"
#include "Order.h"
#include "ScannerSubscription.h"
#include "EPosixClientSocket.h"
#include "EPosixClientSocketPlatform.h"
#include "EWrapper.h"
#include <iostream>
#include <algorithm>
#include <stdio.h>
#include <chrono>
#include <memory>
#include <vector>
#include <deque>
#include <set>
#include <string>
#include <utility>
#include <cassert>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <unordered_map>
#include <stdexcept>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/support/date_time.hpp>

#include <boost/shared_ptr.hpp>

class EPosixClientSocket;

namespace sharp{

// setting initial number of buckets for std::unordered_map, max_load_factor()
// is 1.0 on construction. According to the standard [23.2.5/14], rehashing
// does not occur if the insertion does not cause the container's size to exceed z*B
// where z is the maximum load factor and B the current number of buckets. So as long
// as the watch_list doesn't exceed DEFAULT_BUCKETS_NUM, it should be safe.
static constexpr std::size_t DEFAULT_BUCKETS_NUM = 223; // a prime
static constexpr int PING_DEADLINE = 2; // seconds
static constexpr int SLEEP_BETWEEN_PINGS = 30; // seconds
static constexpr unsigned int MAX_ATTEMPTS = 50;
const auto ATTEMPT_WAITING_TIME = std::chrono::seconds(10);
const auto OPENORDER_WAITING_TIME = std::chrono::milliseconds(100); // 0.10 second
const auto BAR_WAITING_TIME = std::chrono::milliseconds(10000);

template<typename T>
class TypeDisplayer; // usage: TypeDisplayer<decltype(x)>xType;

#define BOOST_LOG_DYN_LINK 1
#define LOG(x) BOOST_LOG_TRIVIAL(x)
void init_logging();

template<typename T>
class sharpdeque: public std::deque<T>
{
public:
    static const std::size_t limit = 10;
    // bool flag = false; // according to the new standard, this is a different memory location with
    // respect to the other parts of this class, thus can be accessed and modified concurrently with
    // respect to the rest.
    std::mutex mx;
    std::condition_variable cv;
    void push(T&& e){  // this is not universal reference, there is no type deduction here, only binds to
    	// rvalue
        if(this->size() >= limit){
        	this->pop_front();
        	LOG(warning)<<"deque limit is reached";
        }
        this->push_back(std::move(e));
    }

    void push(const T & e){
	    if(this->size() >= limit){
	    	this->pop_front();
	    	LOG(warning)<<"deque limit is reached";
	    }
	    this->push_back(e);
    }
};

enum State {
	ST_CONNECT,
	ST_PLACEORDER,
	ST_PLACEORDER_ACK,
	ST_CANCELORDER,
	ST_CANCELORDER_ACK,
	ST_PING,
	ST_PING_ACK,
	ST_IDLE
};

struct OrderResponse
{
	OrderId orderId = -1L;
	int state = -1; // -1: initial value; 0: just placed; -2: canceled
	int clientId = -1;
	int permId = -1;
	int parentId = -1;
	int filled = -1;
	int remaining = -1;
	double avgFillPrice = 0.0;
	double lastFillPrice = 0.0;
	IBString status;
	IBString whyHeld;
};

struct ContractOrder
{
	OrderId orderId;
	Contract contract;
	Order order;
	OrderResponse response;

	ContractOrder():orderId(-1), contract(),order(), response(){};
	ContractOrder(const ContractOrder & contract_order)
	: orderId(contract_order.orderId)
	, contract(contract_order.contract)
	, order(contract_order.order)
	, response(contract_order.response) { }
};

template<typename K, typename V>
typename std::unordered_map<K, std::unique_ptr<V> >::iterator
FindorCreate(K key, std::unordered_map<K, std::unique_ptr<V> > & signal_map){
	// using value_type = V;
	typedef V value_type;
	// using key_type = K;
	auto it = signal_map.find(key);
	if(it != signal_map.end()){
		return it;
	}else{
		auto res = signal_map.insert(std::make_pair(key, std::unique_ptr<value_type>(new value_type())));
		if(res.second){
			return res.first;
		}else{
			assert(0 && "error: signal_map insertion failed !!");
		}
	}
}

struct PlacedOrderContracts
{
	std::unordered_map<OrderId, std::size_t> orderId_index_map;
	std::vector<std::unique_ptr<ContractOrder> > records;

	bool insert(OrderId orderId, ContractOrder & contract_order){
		if(orderId_index_map.count(orderId) > 0){
			LOG(warning)<<"This orderId:"<<orderId<<" has been already registered, records insertion failed.";
			return false;
		}else{
			records.push_back(std::unique_ptr<ContractOrder>(new ContractOrder(contract_order)));
			orderId_index_map[orderId] = records.size() - 1;
			return true;
		}
	}
};

struct ExecutedContract
{
    Contract contract;
    Execution execution;
    CommissionReport report;
    ExecutedContract() = default;
    ExecutedContract(const Contract & c, const Execution & e)
    :contract(c), execution(e){}
};

template <typename T>
class IdType{
private:
	std::atomic<T> id; // TickerId is long = 8 bytes
public:
	IdType(T id_):id(id_){}
	void setInitial(T id_){ id = id_; }
	T getNewId(){ return ++id; }
	T getCurrentId(){ return id; }
};

struct RealTimeBar
{
	TickerId reqId;
	long time;
	double open;
	double low;
	double high;
	double close;
	long volume; // The volume during the time covered by the bar
	double wap;  // The weighted average price during the time covered by the bar.
	int count;   // When TRADES historical data is returned, represents the number of
	             // trades that occurred during the time period the bar covers.
	RealTimeBar(TickerId r, long t, double o, double l, double h,
		double c, long v, double w, int ct): reqId(r), time(t), open(o),
		low(l), high(h), close(c), volume(v), wap(w), count(ct){}
};

struct OptionPosition
 {
    long conId = -1L;
    int32_t position = 0;
    double avgCost = 0.0;
    Contract contract;
 };

struct Asset
{
    Contract contract;
    int32_t position;
    double marketPrice;
    double marketValue;
    double averageCost;
    double unrealizedPNL;
    double realizedPNL;
    double accountName;
};

class EWrapperImpl : public EWrapper
{
public:
	EWrapperImpl( const std::vector<std::string> wl = std::vector<std::string>() );
	~EWrapperImpl();

public:
	bool connect(const char * host, unsigned int port, int clientId = 0);
	void disconnect() const;
	bool isConnected() const;

public:
	void monitor();

    // order related
	void placeOrder();
	bool cancelOrder(OrderId orderId);
	void reqOpenOrders();    // orders placed by api
	void reqAllOpenOrders(); // orders placed by both api and tws
	bool reqGlobalCancel();  // cancel orders placed by both api and tws
    int reqExecutions(const ExecutionFilter & ef);

	// watchlist, market data related
	void reqMarketSnapshot(); // TODO
	// calling this function will call
	// m_pClient->reqRealTimeBars, otherwise it doesn't make sense to call this function.
	bool addToWatchList( const std::vector<std::string> &);
	bool removeFromWatchList(const std::vector<std::string> &);
	bool removeZombieSymbols(const std::vector<std::string> &);
	bool reqRealTimeBars();
	// getNextBar is done at thrift level

    // account, portfolio, position related
    void reqAccountUpdates(bool subscribe, const std::string & acctCode);
    void reqPositions();

    bool checkValidId( OrderId orderId);
	std::string getField(TickType tickType);

private:
	void reqCurrentTime();

public:
	// events
	void tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute);
	void tickSize(TickerId tickerId, TickType field, int size);
	void tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
		double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice);
	void tickGeneric(TickerId tickerId, TickType tickType, double value);
	void tickString(TickerId tickerId, TickType tickType, const IBString& value);
	void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
		double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry);
	void orderStatus(OrderId orderId, const IBString &status, int filled,
		int remaining, double avgFillPrice, int permId, int parentId,
		double lastFillPrice, int clientId, const IBString& whyHeld);
	void openOrder(OrderId orderId, const Contract&, const Order&, const OrderState&);
	void openOrderEnd();
	void winError(const IBString &str, int lastError);
	void connectionClosed();
	void updateAccountValue(const IBString& key, const IBString& val,
		const IBString& currency, const IBString& accountName);
	void updatePortfolio(const Contract& contract, int position,
		double marketPrice, double marketValue, double averageCost,
		double unrealizedPNL, double realizedPNL, const IBString& accountName);
	void updateAccountTime(const IBString& timeStamp);
	void accountDownloadEnd(const IBString& accountName);
	void nextValidId(OrderId orderId);
	void contractDetails(int reqId, const ContractDetails& contractDetails);
	void bondContractDetails(int reqId, const ContractDetails& contractDetails);
	void contractDetailsEnd(int reqId);
	void execDetails(int reqId, const Contract& contract, const Execution& execution);
	void execDetailsEnd(int reqId);
	void error(const int id, const int errorCode, const IBString errorString);
	void updateMktDepth(TickerId id, int position, int operation, int side,
		double price, int size);
	void updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
		int side, double price, int size);
	void updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch);
	void managedAccounts(const IBString& accountsList);
	void receiveFA(faDataType pFaDataType, const IBString& cxml);
	void historicalData(TickerId reqId, const IBString& date, double open, double high,
		double low, double close, int volume, int barCount, double WAP, int hasGaps);
	void scannerParameters(const IBString &xml);
	void scannerData(int reqId, int rank, const ContractDetails &contractDetails,
		const IBString &distance, const IBString &benchmark, const IBString &projection,
		const IBString &legsStr);
	void scannerDataEnd(int reqId);
	void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
		long volume, double wap, int count);
	void currentTime(long time);
	void fundamentalData(TickerId reqId, const IBString& data);
	void deltaNeutralValidation(int reqId, const UnderComp& underComp);
	void tickSnapshotEnd(int reqId);
	void marketDataType(TickerId reqId, int marketDataType);
	void commissionReport( const CommissionReport& commissionReport);
	void position( const IBString& account, const Contract& contract, int position, double avgCost);
	void positionEnd();
	void accountSummary( int reqId, const IBString& account, const IBString& tag, const IBString& value, const IBString& curency);
	void accountSummaryEnd( int reqId);
	void verifyMessageAPI( const IBString& apiData);
	void verifyCompleted( bool isSuccessful, const IBString& errorText);
	void displayGroupList( int reqId, const IBString& groups);
	void displayGroupUpdated( int reqId, const IBString& contractInfo);

private:
	std::unique_ptr<EPosixClientSocket> m_pClient;
	State m_state;
	time_t m_sleepDeadline;

public:
	OrderId m_orderId; // long

public:
	std::string host;
	unsigned int port;
	int clientId;
    std::mutex mutex; // mutex for order statuses

    IdType<TickerId> ticker_id{-1L};
    IdType<OrderId> order_id{-1L};
    IdType<int> req_id{1};
    std::atomic<bool> open_order_flag;
	std::set<OrderId> open_order_set;

    ContractOrder contract_order_request;
	// placed_contract_orders is written and read by several EWrapperImpl member functions
	// and thrift handler functions, these functions should be synchronized.
	PlacedOrderContracts placed_contract_orders;
	std::vector<OrderId> used_order_ids;

    std::unordered_map<int, std::pair<std::atomic<bool>,std::set<std::string>>>requested_execs;
    std::unordered_map<std::string, ExecutedContract> received_execs; // key is execId

    std::unordered_map<std::string, TickerId>watch_list;
	// non-const operation on std::deque is not thread-safe,
	// so the following needs to be synchronized.
	std::unordered_map<TickerId, std::unique_ptr< sharpdeque<RealTimeBar>>>watch_list_bars;

    // account, portfolio, position related
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> accounts;
    // stk_positions are symbol based, opt_positions are conId based.
    // portfolio is conId based. conId is long.
    std::atomic<bool> position_flag;
    std::atomic<bool> account_flag;
    // std::pair<position, avgCost> for stk_positions
    std::unordered_map<std::string, std::unordered_map<std::string, std::pair<int, double>>> stk_positions;
    std::unordered_map<std::string, std::unordered_map<long, OptionPosition>> opt_positions;
    std::unordered_map<std::string, std::unordered_map<long, Asset>> portfolio;

};

void run_server (EWrapperImpl & ibtrader);

class SharpClientService{
public:
	virtual ~SharpClientService() = default;
	virtual void ping () = 0;
	virtual void placeOrder(OrderResponse& response, const Contract& c_request, const Order& o_request) = 0;
	virtual int64_t getOrderID() = 0;
	virtual void cancelOrder(OrderResponse & response, const int64_t o_id) = 0;
	virtual void getOrderStatus(ContractOrder & co, const int64_t o_id) = 0;
};

SharpClientService *make_client ();


} // end of namespace sharp
#endif

