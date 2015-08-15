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

inline void translate_realtimebar(const RealTimeBar & tb, api::RealTimeBar & time_bar);
inline void translate_orderstatus(const ContractOrder & co, api::OrderStatus & os );

class SharpHandler : virtual public api::SharpIf {

    EWrapperImpl & trader;
    const RealTimeBar invalid_bar = RealTimeBar(-2L, 1L, 1.0, 1.0, 1.0, 1.0, 1L, 1.0, 1);
    bool bar_requested;

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
    SharpHandler(EWrapperImpl & trader_): trader(trader_), bar_requested(false){}

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

            std::lock_guard<std::mutex> lk(trader.mutex);
            auto & m = trader.placed_contract_orders.orderId_index_map;
            auto & rds = trader.placed_contract_orders.records;
            auto & resp = rds[m.at(m_req.orderId)]->response;

            assert(m_req.orderId == resp.orderId && "error: assert in SharpHandler::placeOrder: m_req.orderId == resp.orderId failed." );
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
                std::lock_guard<std::mutex> lk(trader.mutex);
                auto & m = trader.placed_contract_orders.orderId_index_map;
                auto & rds = trader.placed_contract_orders.records;
                auto & resp = rds[m.at(o_id)]->response;
                assert(o_id == resp.orderId && "error: assert in SharpHandler::cancelOrder: o_id == resp.orderId failed.");
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

    void getOrderStatus(api::OrderStatus & os, const int64_t o_id){
        protect( [this, &os, o_id](){
            std::lock_guard<std::mutex> lk(trader.mutex);
            auto & m = trader.placed_contract_orders.orderId_index_map;
            auto & rds = trader.placed_contract_orders.records;
            try{
                auto index = m.at(o_id);
                auto & co = *(rds[index]);
                assert(o_id == co.response.orderId && "error: assert in SharpHandler::orderStatus: o_id == response.orderId failed.");
                translate_orderstatus(co, os);
                return;
            }catch(...){
                os.state = -2; // orderId doesn't exist
                return;
            }
        } );
    }

    // modern compilers make exceptions usually faster
    // on the non-exceptional path compared to error code handling.
    void reqOpens(std::vector<api::OrderStatus> & opens){
        while(!trader.open_order_flag.load(std::memory_order_relaxed)){
            std::this_thread::sleep_for(OPENORDER_WAITING_TIME);
        }
        opens.clear();
        std::lock_guard<std::mutex> lk(trader.mutex);
        auto & m = trader.placed_contract_orders.orderId_index_map;
        auto & rds = trader.placed_contract_orders.records;
        for(auto e : trader.open_order_set){
            std::size_t index;
            try{
                index = m.at(e);
            }catch(...){
                LOG(error)<<"Why this orderId "<<e<<" is not inserted into placed_contract_orders?";
                continue;
            }
            auto & co = *(rds[index]);
            api::OrderStatus os;
            translate_orderstatus(co, os);
            opens.push_back(os);
        }
    }

    // this function must be called by at most one thread on python client side
    void reqOpenOrders(std::vector<api::OrderStatus> & opens){
        protect( [this, &opens](){
            trader.reqOpenOrders();
            reqOpens(opens);
        } );
    }

    void reqAllOpenOrders(std::vector<api::OrderStatus> & opens){
        protect( [this, &opens](){
            trader.reqAllOpenOrders();
            reqOpens(opens);
        } );
    }

    void reqGlobalCancel(){
        protect( [this](){
            trader.reqGlobalCancel();
        } );
    }

    void requestRealTimeBars(){
        if(!bar_requested){
            protect( [this]() {
            trader.requestRealTimeBars();
            } );
            bar_requested = true;
        }
    }

    void addToWatchList(const std::vector<std::string> & wl){
        protect( [this, &wl](){
            trader.addToWatchList(wl);
        } );
    }

    // on the python client side, this function is called after all the getNextBar
    // requests (for the to-be-removed watch list) are done (i.e, after join is called.)
    void removeFromWatchList(const std::vector<std::string> & rm){
        protect( [this, &rm](){
            trader.removeFromWatchList(rm); // will cancel the request for real time bars
        } );
    }

    void removeZombieSymbols(const std::vector<std::string> & rm){
        protect( [this, &rm](){
            trader.removeZombieSymbols(rm); // will cancel the request for real time bars
        } );
    }

    // if C++ thrift server shuts down (the watch_list and watch_list_bars are gone),
    // while the python client is still running, C++ thrift server restarts again,
    // the python client will get invalid_bar for all the getNextBar requests, the python
    // client has to restart again.

    // if python client shuts down while C++ thrift server is still running, and python client
    // restarts again (with possibly modified watch list), addToWatchList will work fine, since it
    // only selects the symbols which are not in the watch_list, and only send reqRealTimeBars for
    // those symbols. But the symbols which are not in the new watch list (by the python client side)
    // will not get deleted, since there is no request to removeFromWatchList and right now I don't
    // have a solution to check if there is python client connected.

    // during the time both C++ thrift server and python client are running, if addToWatchList is
    // called, watch_list will get modified, at the same time, however, it is not safe to access
    // other exsisting symbols in watch_list if watch_list is std::map. std::unordered_map will
    // probably work fine, since all possible symbols (A - ZZZZ) have been tested to be hashed
    // to different values. Thus adding buckets will not affect the existing buckets. This may
    // be subject to change due to compiler updates, etc. Another alternative without explicitly
    // using lock is concurrent_unordered_map or lock-free maps.
    // after addToWatchList returns on the python client side, the new sybmol is already in
    // watch_list.

    // if getNextBar is called, that means the python client is running, which means that
    // addToWatchList has already been called, which means symbol should be in watch_list,
    // if symbol is not, that means C++ thrift server has shut down previously during the
    // python client is running, so the watch_list must be empty().

    void getNextBar(api::RealTimeBar& next_bar, const std::string& symbol){
        protect([this, &next_bar, &symbol](){
            long id; // TickerId is long
            try{
                id = trader.watch_list.at(symbol);
            }catch(...){
                translate_realtimebar(invalid_bar, next_bar);
                return;
            }
            auto & bars = trader.watch_list_bars[id];
            auto start = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start);

            while(bars.empty() && elapsed < BAR_WAITING_TIME){
                elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start);
            }

            if(!bars.empty()){
                std::lock_guard<std::mutex> lk(trader.bar_mutexes[id]);
                translate_realtimebar(bars.front(), next_bar);
                bars.pop_front();
            }else{
                LOG(info)<<"timeout for "<<symbol;
                translate_realtimebar(invalid_bar, next_bar);
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
        ibtrader.connect( ibtrader.host.c_str(), ibtrader.port, ibtrader.clientId);
        // ibtrader.reqMarketSnapshot();

        while( ibtrader.isConnected() ) {
            ibtrader.monitor();
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

inline void translate_realtimebar(const RealTimeBar & tb, api::RealTimeBar & time_bar){
    time_bar.reqId = tb.reqId;
    time_bar.time = tb.time;
    time_bar.open = tb.open;
    time_bar.low = tb.low;
    time_bar.high = tb.high;
    time_bar.close = tb.close;
    time_bar.volume = tb.volume;
    time_bar.wap = tb.wap;
    time_bar.count = tb.count;
}

inline void translate_request(const Contract & c_request, const Order & o_request,
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

inline void translate_response(OrderResponse & response, const api::OrderResponse & resp ){

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

inline void translate_orderstatus(const ContractOrder & co, api::OrderStatus & os ){
    os.symbol = co.contract.symbol;
    os.secType = co.contract.secType;
    os.exchange = co.contract.exchange;
    os.currency = co.contract.currency;

    os.action = co.order.action;
    os.totalQuantity = co.order.totalQuantity;
    os. orderType = co.order.orderType;
    os. lmtPrice = co.order.lmtPrice;

    os.orderId = co.response.orderId;
    os.state = co.response.state;
    os.clientId = co.response.clientId;
    os.permId = co.response.permId;
    os.parentId = co.response.parentId;
    os.filled = co.response.filled;
    os.remaining = co.response.remaining;
    os.avgFillPrice = co.response.avgFillPrice;
    os.lastFillPrice = co.response.lastFillPrice;
    os.status = co.response.status;
    os.whyHeld = co.response.whyHeld;
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

    void getOrderStatus(ContractOrder & co, const int64_t o_id){
        protect([this, &co, o_id](){
            api::OrderStatus os;
            client.getOrderStatus(os, o_id);
        } );
    }


};

SharpClientService *make_client () {
    return new SharpClientImpl();
}




} // end of namespace sharp
