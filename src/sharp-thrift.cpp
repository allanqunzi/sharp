#include <future>
#include <thread>
//#include <boost/log/utility/setup/console.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include "sharp.h"
#include "sharp_helper.h"
#include "./thrift/Sharp.h"

namespace sharp{

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::server;

class SharpHandler : virtual public api::SharpIf {

    EWrapperImpl & trader;

    void protect (std::function<void()> const &callback) {
        try {
            callback();
        }
        catch (Error const &e) {
            api::Exception ae;
            ae.what = e.code();
            ae.why = e.what();
            throw ae;
        }
        catch (std::exception const &e) {
            api::Exception ae;
            ae.what = ErrorCode_Unknown;
            ae.why = e.what();
            throw ae;
        }
        catch (...) {
            api::Exception ae;
            ae.what = ErrorCode_Unknown;
            throw ae;
        }
    }
public:
    SharpHandler(EWrapperImpl & trader_): trader(trader_){}

    void ping(api::PingResponse& response, const api::PingRequest& request) {
    }

    void placeOrder(api::OrderResponse& o_response, const api::ContractRequest& c_req,
        const api::OrderRequest& o_req){

        protect( [this, &o_response, c_req, o_req](){

            auto & m_req = trader.contract_order_request;

            m_req.orderId = trader.m_orderId;

            m_req.contract.symbol = c_req.symbol;
            m_req.contract.secType = c_req.secType;
            m_req.contract.exchange = c_req.exchange;
            m_req.contract.currency = c_req.currency;

            m_req.order.action = o_req.action;
            m_req.order.totalQuantity = o_req.totalQuantity;
            m_req.order.orderType = o_req.orderType;
            m_req.order.lmtPrice = o_req.lmtPrice;

            trader.placeOrder();

            auto & m = trader.placed_contract_orders.orderId_index_map;
            auto & rds = trader.placed_contract_orders.records;
            auto & resp = rds[m.at(m_req.orderId)]->response;

            assert(m_req.orderId == resp.orderId && "assert in SharpHandler::placeOrder: m_req.orderId == resp.orderId failed." );
            o_response.orderId = m_req.orderId;
            o_response.state = resp.state;

            if(o_response.state == 0){
                return;
            }else{
                o_response.clientId = resp.clientId;
                o_response.permId = resp.permId;
                o_response.parentId = resp.parentId;
                o_response.filled = resp.filled;
                o_response.remaining = resp.remaining;
                o_response.avgFillPrice = resp.avgFillPrice;
                o_response.lastFillPrice = resp.lastFillPrice;
                o_response.status = resp.status;
                o_response.whyHeld = resp.whyHeld;
            }

        } );
    }

    int64_t getOrderID(){
        return trader.m_orderId;
    }

    void cancelOrder(api::OrderResponse& o_response, const int64_t o_id){

        protect( [this, &o_response, o_id](){

            auto ret = trader.cancelOrder(o_id);
            o_response.orderId = o_id;
            if(ret){ // order canceled successfully
                auto & m = trader.placed_contract_orders.orderId_index_map;
                auto & rds = trader.placed_contract_orders.records;
                auto & resp = rds[m.at(o_id)]->response;
                assert(o_id == resp.orderId && "assert in SharpHandler::cancelOrder: o_id == resp.orderId failed.");
                o_response.state = -1;
                o_response.clientId = resp.clientId;
                o_response.permId = resp.permId;
                o_response.parentId = resp.parentId;
                o_response.filled = resp.filled;
                o_response.remaining = resp.remaining;
                o_response.avgFillPrice = resp.avgFillPrice;
                o_response.lastFillPrice = resp.lastFillPrice;
                o_response.status = resp.status;
                o_response.whyHeld = resp.whyHeld;
            }else{
                o_response.state = -2; // orderId doesn't exist
            }

        } );
    }

    void orderStatus(api::OrderResponse& o_response, const int64_t o_id){
        protect( [this, &o_response, o_id](){

            auto & m = trader.placed_contract_orders.orderId_index_map;
            if(m.count(o_id) == 0){
                o_response.state = -2; // orderId doesn't exist
                return;
            }else{
                auto & rds = trader.placed_contract_orders.records;
                auto & resp = rds[m.at(o_id)]->response;
                assert(o_id == resp.orderId && "assert in SharpHandler::orderStatus: o_id == resp.orderId failed.");
                o_response.orderId = o_id;
                o_response.state = resp.state;
                o_response.clientId = resp.clientId;
                o_response.permId = resp.permId;
                o_response.parentId = resp.parentId;
                o_response.filled = resp.filled;
                o_response.remaining = resp.remaining;
                o_response.avgFillPrice = resp.avgFillPrice;
                o_response.lastFillPrice = resp.lastFillPrice;
                o_response.status = resp.status;
                o_response.whyHeld = resp.whyHeld;
            }
        } );
    }
};

void run_server (EWrapperImpl & ibtrader) {
    std::cout << "Starting the server..."<<std::endl;
    int worker_count = 4;
    boost::shared_ptr<TProtocolFactory> apiFactory(new TBinaryProtocolFactory());
    //boost::shared_ptr<TProtocolFactory> apiFactory(new TCompactProtocolFactory());
    boost::shared_ptr<SharpHandler> handler(new SharpHandler(ibtrader));
    boost::shared_ptr<TProcessor> processor(new api::SharpProcessor(handler));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(9090));
    boost::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());

    boost::shared_ptr<ThreadManager> threadManager = ThreadManager::newSimpleThreadManager(worker_count);
    boost::shared_ptr<PosixThreadFactory> threadFactory = boost::shared_ptr<PosixThreadFactory>(new PosixThreadFactory());
    threadManager->threadFactory(threadFactory);
    threadManager->start();
    TThreadPoolServer server(processor, serverTransport, transportFactory, apiFactory, threadManager);

    //TSimpleServer server(processor, serverTransport, transportFactory, apiFactory);

    std::future<void> ret = std::async(std::launch::async, [&server](){server.serve();});

    unsigned attempt = 0;
    for (;;)
    {
        ++attempt;
        ibtrader.connect( ibtrader.host, ibtrader.port, ibtrader.clientId);
        while( ibtrader.isConnected() ) {
            ibtrader.monitor();
            std::this_thread::sleep_for(MONITOR_WAITING_TIME);
        }
        if( attempt >= MAX_ATTEMPTS) {
            break;
        }
        std::this_thread::sleep_for(ATTEMPT_WAITING_TIME);
        //WaitSignal ws;
        //int sig = 0;
        //ws.wait(&sig);
    }
    server.stop();
    ret.get();

}

void translate_request(const Contract & c_request, const Order & o_request,
    api::ContractRequest & c_req, api::OrderRequest & o_req){

    c_req.symbol = c_request.symbol;
    c_req.secType = c_request.secType;
    c_req.exchange = c_request.exchange;
    c_req.currency = c_request.currency;

    o_req.action = o_request.action;
    o_req.totalQuantity = o_request.totalQuantity;
    o_req.orderType = o_request.orderType;
    o_req.lmtPrice = o_request.lmtPrice;
}

void translate_response(OrderResponse & response, const api::OrderResponse & resp ){

    response.orderId = resp.orderId;
    response.state = resp.state;
    response.clientId = resp.clientId;
    response.permId = resp.permId;
    response.parentId = resp.parentId;
    response.filled = resp.filled;
    response.remaining = resp.remaining;
    response.avgFillPrice = resp.avgFillPrice;
    response.lastFillPrice = resp.lastFillPrice;
    response.status = resp.status;
    response.whyHeld = resp.whyHeld;
}


class SharpClientImpl: public SharpClientService
{
    mutable boost::shared_ptr<TTransport> socket;
    mutable boost::shared_ptr<TTransport> transport;
    mutable boost::shared_ptr<TProtocol> protocol;
    mutable api::SharpClient client;

    void protect (std::function<void()> const &callback) {
        try {
            callback();
        }
        catch (api::Exception const &ae) {
            throw Error(ae.why, ae.what);
        }
        catch (...) {
            throw Error("unknown error");
        }
    }

    void protect (std::function<void()> const &callback) const {
        try {
            callback();
        }
        catch (api::Exception const &ae) {
            throw Error(ae.why, ae.what);
        }
        catch (...) {
            throw Error("unknown error");
        }
    }
public:
    SharpClientImpl()
    : socket(new TSocket("localhost", 9090)),
    transport(new TBufferedTransport(socket)),
    protocol(new TBinaryProtocol(transport)),
    //protocol(new TCompactProtocol(transport)),
    client(protocol)
    {
        transport->open();
    }
    ~SharpClientImpl(){
        transport->close();
    };

    void ping () {
        api::PingRequest req;
        api::PingResponse resp;
        client.ping(resp, req);
    }

    void placeOrder(OrderResponse& response, const Contract& c_request,
        const Order& o_request){
        protect( [this, &response, &c_request, &o_request](){
            api::OrderResponse resp;
            api::ContractRequest c_req;
            api::OrderRequest o_req;

            translate_request(c_request, o_request, c_req, o_req);
            client.placeOrder(resp, c_req, o_req);
            translate_response(response, resp);
        } );
    }

    int64_t getOrderID(){
        return client.getOrderID();
    }

    void cancelOrder(OrderResponse & response, const int64_t o_id){
        protect( [this, &response, o_id](){
            api::OrderResponse resp;
            client.cancelOrder(resp, o_id);
            translate_response(response, resp);
        } );
    }

    void orderStatus(OrderResponse & response, const int64_t o_id){
        protect([this, &response, o_id](){
            api::OrderResponse resp;
            client.orderStatus(resp, o_id);
            translate_response(response, resp);
        } );
    }


};

SharpClientService *make_client () {
    return new SharpClientImpl();
}




} // end of namespace sharp