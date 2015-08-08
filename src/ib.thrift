namespace cpp algotrade.api

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
    2:required string action;
    3:required i64 totalQuantity;
    4:required string orderType;
    5:required double lmtPrice;
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

exception Exception {
      1: i32 what;
      2: string why;
}

service AlgoTrade {
    PingResponse ping (1:required PingRequest request);
    i64 getOrderID();
    OrderResponse placeOrder (1:required ContractRequest c_req, 2:required OrderRequest o_req) throws (1:Exception e);
    OrderResponse cancelOrder (1:required i64 o_id) throws (1:Exception e);
    OrderResponse orderStatus (1:required i64 o_id) throws (1:Exception e);
}

