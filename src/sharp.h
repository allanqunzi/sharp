/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#ifndef QUNZI_SHARP_HEADER
#define QUNZI_SHARP_HEADER

#include "CommonDefs.h"
#include "Contract.h"
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
#include <string>
#include <utility>
#include <cassert>
#include <mutex>
#include <map>
#include <boost/log/trivial.hpp>
#include <boost/log/attributes/named_scope.hpp>
class EPosixClientSocket;

namespace sharp{

static constexpr int PING_DEADLINE = 2; // seconds
static constexpr int SLEEP_BETWEEN_PINGS = 30; // seconds
static constexpr unsigned int MAX_ATTEMPTS = 50;
const auto ATTEMPT_WAITING_TIME = std::chrono::seconds(10);
const auto MONITOR_WAITING_TIME = std::chrono::milliseconds(50); // 0.05 second
const auto BAR_WAITING_TIME = std::chrono::milliseconds(5000);

template<typename T>
class TypeDisplayer; // usage: TypeDisplayer<decltype(x)>xType;

#define BOOST_LOG_DYN_LINK 1
#define LOG(x) BOOST_LOG_TRIVIAL(x)

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
	ContractOrder(const ContractOrder & contract_order):response(contract_order.response){
		orderId = contract_order.orderId;

		contract.symbol = contract_order.contract.symbol;
		contract.secType = contract_order.contract.secType;
		contract.exchange = contract_order.contract.exchange;
		contract.currency = contract_order.contract.currency;

		order.action = contract_order.order.action;
		order.totalQuantity = contract_order.order.totalQuantity;
		order.orderType = contract_order.order.orderType;
		order.lmtPrice = contract_order.order.lmtPrice;
	}
};

template<typename K, typename V>
typename std::map<K, std::unique_ptr<V> >::iterator
FindorCreate(K key, std::map<K, std::unique_ptr<V> > & signal_map){
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
			assert(0 && "signal_map insertion failed !!");
		}
	}
}

struct PlacedOrderContracts
{
	std::map<OrderId, std::size_t> orderId_index_map;
	std::vector<std::unique_ptr<ContractOrder> > records;

	bool insert(OrderId orderId, ContractOrder & contract_order){
		if(orderId_index_map.count(orderId) > 0){
			std::cout<<"This orderId:"<<orderId<<" has been already registered, records insertion failed."<<std::endl;
			return false;
		}else{
			records.push_back(std::unique_ptr<ContractOrder>(new ContractOrder(contract_order)));
			orderId_index_map[orderId] = records.size() - 1;
			// auto & resp = records.back()->response;
			// resp.orderId = orderId;
			return true;
		}
	}
};

class IdType{
private:
	TickerId id = -1L; // TickerId is long = 8 bytes
public:
	TickerId getNewId(){ return ++id; }
	TickerId getCurrentId(){ return id;	}
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
	void placeOrder();
	bool cancelOrder(OrderId orderId);
	void reqMarketSnapshot();
	bool checkValidId( OrderId orderId);
	// calling this function will call
	// m_pClient->reqRealTimeBars, otherwise it doesn't make sense to call this function.
	bool addToWatchList( const std::vector<std::string> &);

	bool removeFromWatchList(const std::vector<std::string> &);
	bool requestRealTimeBars();

	// getNextBar is done at thrift level

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
	OrderId m_orderId;

public:
	std::string host;
	unsigned int port;
	int clientId;
	std::mutex mutex;
	ContractOrder contract_order_request;
	// placed_contract_orders is written by three member functions: placeOrder, cancelOrder and orderStatus,
	// so these functions should be synchronized.
	PlacedOrderContracts placed_contract_orders;
	std::vector<OrderId> order_ids; // written only by EWrapperImpl::nextValidId( OrderId orderId)
	IdType ticker_id;
	std::map<std::string, TickerId> watch_list;
	std::map<TickerId, std::deque<RealTimeBar> >watch_list_bars;
};


void run_server (EWrapperImpl & ibtrader);

class SharpClientService{
public:
	virtual ~SharpClientService() = default;
	virtual void ping () = 0;
	virtual void placeOrder(OrderResponse& response, const Contract& c_request, const Order& o_request) = 0;
	virtual int64_t getOrderID() = 0;
	virtual void cancelOrder(OrderResponse & response, const int64_t o_id) = 0;
	virtual void orderStatus(OrderResponse & response, const int64_t o_id) = 0;
};

SharpClientService *make_client ();



} // end of namespace sharp
#endif

