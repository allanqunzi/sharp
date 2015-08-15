namespace cpp sharp.api

struct PingRequest {
}

struct PingResponse {
}

struct ContractRequest {
    1:required string symbol;
    2:required string secType;
    3:required string exchange;
    4:required string currency;
}

struct OrderRequest {
    1:required string action;
    2:required i64 totalQuantity;
    3:required string orderType;
    4:required double lmtPrice;
}

struct OrderResponse {
    1:required i64 orderId;
    2:required i16 state;
    3:optional i32 clientId = 0;
    4:optional i32 permId = 0;
    5:optional i32 parentId = 0;
    6:optional i32 filled = 0;
    7:optional i32 remaining = 0;
    8:optional double avgFillPrice = 0.0;
    9:optional double lastFillPrice = 0.0;
    10:optional string status = "";
    11:optional string whyHeld = "";
}

/* asking about order status only returning OrderResponse in not sufficient,
* need to know contract order details also, although thrift seems to support
* nested structs, working with flattened out data is safer.
*/
struct OrderStatus {

/*
    1:required ContractRequest ctrt;
    2:required OrderRequest ord;
    3:required OrderResponse oresp;
*/
    // contract part
    1:required string symbol;
    2:required string secType;
    3:required string exchange;
    4:required string currency;

    // order part
    5:required string action;
    6:required i64 totalQuantity;
    7:required string orderType;
    8:required double lmtPrice;

    // response part
    9:required i64 orderId;
    10:required i16 state;
    11:optional i32 clientId = 0;
    12:optional i32 permId = 0;
    13:optional i32 parentId = 0;
    14:optional i32 filled = 0;
    15:optional i32 remaining = 0;
    16:optional double avgFillPrice = 0.0;
    17:optional double lastFillPrice = 0.0;
    18:optional string status = "";
    19:optional string whyHeld = "";
}

struct RealTimeBar {
    1:required i64 reqId = -2;
    2:required i64 time = 1;
    3:required double open = 1.0;
    4:required double low = 1.0;
    5:required double high = 1.0;
    6:required double close = 1.0;
    7:required i64 volume = 1;
    8:required double wap = 1.0;
    9:required i32 count = 1;
}

exception Exception {
      1: i32 what;
      2: string why;
}

service Sharp {
    PingResponse ping (1:required PingRequest request);
    i64 getOrderID();
    OrderResponse placeOrder (1:required ContractRequest c_req, 2:required OrderRequest o_req) throws (1:Exception e);
    OrderResponse cancelOrder (1:required i64 o_id) throws (1:Exception e);
    OrderStatus getOrderStatus (1:required i64 o_id) throws (1:Exception e);
    list<OrderStatus> reqOpenOrders() throws (1:Exception e);
    list<OrderStatus> reqAllOpenOrders() throws (1:Exception e);
    void reqGlobalCancel() throws (1:Exception e);

    void requestRealTimeBars() throws (1:Exception e);
    void addToWatchList(1:required list<string> wl) throws (1:Exception e);
    void removeFromWatchList(1:required list<string> rm) throws (1:Exception e);
    void removeZombieSymbols(1:required list<string> rm) throws (1:Exception e);
    RealTimeBar getNextBar(1:required string symbol) throws (1:Exception e);
}

