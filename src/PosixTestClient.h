/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#ifndef posixtestclient_h__INCLUDED
#define posixtestclient_h__INCLUDED

#include "Contract.h"
#include "Order.h"
#include "EWrapper.h"
#include <stdio.h>
#include <memory>
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <utility>
#include <cassert>
#include <mutex>

class EPosixClientSocket;

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
struct ContractOrder
{
	OrderId orderId;
	Contract contract;
	Order order;

	ContractOrder():orderId(-1), contract(),order(){};
	ContractOrder(const ContractOrder & contract_order){
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

struct PlacedOrderContracts
{
	std::map<OrderId, int> orderId_index_map;
	std::vector<std::unique_ptr<ContractOrder> > records;

	void update(OrderId orderId, ContractOrder & contract_order){
		if(orderId_index_map.count(orderId) > 0){
			std::cout<<"This orderId:"<<orderId<<" has been already registered, records insertion failed."<<std::endl;
			return;
		}else{
			records.push_back(std::unique_ptr<ContractOrder>(new ContractOrder(contract_order)));
			orderId_index_map[orderId] = records.size()-1;
			return;
		}
	}
};

struct OrderResponse
{
	OrderId orderId;
	int16_t state;
	IBString status;
	int filled;
	int remaining;
	double avgFillPrice;
	int permId;
	int parentId;
	double lastFillPrice;
	int clientId;
	IBString whyHeld;
};

/* the following is by Aiqun Huang */
template<typename K, typename V>
typename std::map<K, std::unique_ptr<V> >::iterator
FindorCreate(K key, std::map<K, std::unique_ptr<V> > & signal_map){
	//using value_type = V;
	typedef V value_type;
	//using key_type = K;
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

class PosixTestClient : public EWrapper
{
public:

	PosixTestClient();
	~PosixTestClient();

	void monitor();

public:

	bool connect(const char * host, unsigned int port, int clientId = 0);
	void disconnect() const;
	bool isConnected() const;
public: // defined by Aiqun Huang
    void placeOrder(ContractOrder &);
    bool checkValidId( OrderId orderId);
	bool cancelOrder(OrderId orderId);

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

	std::auto_ptr<EPosixClientSocket> m_pClient;
	State m_state;
	time_t m_sleepDeadline;

public:
	OrderId m_orderId;

	// the following is defined by Aiqun Huang
public:
	const char *host;
	unsigned int port;
	int clientId;
	std::mutex mutex;
	// order_statuses is written by three member functions: placeOrder, cancelOrder and orderStatus,
	// so these functions should be synchronized.
	std::map<OrderId, std::unique_ptr<OrderResponse> > order_statuses;
	ContractOrder contract_order_request;
	PlacedOrderContracts placed_contract_orders; // written only by PosixTestClient::placeOrder(ContractOrder & contract_order)
	std::vector<OrderId> order_ids; // written only by PosixTestClient::nextValidId( OrderId orderId)
};

namespace algotrade{
	void run_server (PosixTestClient & ibtrader);

	class AlgoTradeClientService{
	public:
		virtual ~AlgoTradeClientService() = default;
		virtual void ping () = 0;
		virtual void placeOrder(OrderResponse& response, const Contract& c_request, const Order& o_request) = 0;
        virtual int64_t getOrderID() = 0;
        virtual void cancelOrder(OrderResponse & response, const int64_t o_id) = 0;
        virtual void orderStatus(OrderResponse & response, const int64_t o_id) = 0;
	};

	AlgoTradeClientService *make_client ();
}

#endif

