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
    3:optional string status = "";
    4:optional i32 filled = 0;
    5:optional i32 remaining = 0;
    6:optional double avgFillPrice = 0.0;
    7:optional i32 permId = 0;
    8:optional i32 parentId = 0;
    9:optional double lastFillPrice = 0.0;
    10:optional i32 clientId = 0;
    11:optional string whyHeld = "";
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
    OrderResponse orderStatus (1:required i64 o_id) throws (1:Exception e);

    bool requestRealTimeBars() throws (1:Exception e);
    bool addToWatchList(1:required list<string> wl) throws (1:Exception e);
    bool removeFromWatchList(1:required list<string> rm) throws (1:Exception e);
    RealTimeBar getNextBar(1:required string symbol) throws (1:Exception e);
}

