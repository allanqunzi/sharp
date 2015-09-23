namespace cpp sharp.api

struct PingRequest{
}

struct PingResponse{
}

struct ContractRequest{
    1:required string symbol;
    2:required string secType;
    3:required string exchange;
    4:required string currency;
}

struct OrderRequest{
    1:required string action;
    2:required i64 totalQuantity;
    3:required string orderType;
    4:required double lmtPrice;
}

struct OrderResponse{
    1:required i64 orderId;
    2:required i32 state;
    3:required i32 clientId = 0;
    4:required i32 permId = 0;
    5:required i32 parentId = 0;
    6:required i32 filled = 0;
    7:required i32 remaining = 0;
    8:required double avgFillPrice = 0.0;
    9:required double lastFillPrice = 0.0;
    10:required string status = "";
    11:required string whyHeld = "";
}

/* asking about order status only returning OrderResponse in not sufficient,
* need to know contract order details also, although thrift seems to support
* nested structs, working with flattened out data is safer.
*/
struct OrderStatus{

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

struct ExecutionFilter{
    1:required i64 clientId = 0;
    2:required string acctCode;
    3:required string time;
    4:required string symbol;
    5:required string secType;
    6:required string exchange;
    7:required string side;
}

struct ExecutedContract{

    1:required string symbol = "";
    2:required string secType = "";
    3:required string expiry = "";
    4:required string right = "";
    5:required string multiplier = "";
    6:required string exchange;
    7:required string primaryExchange = "";
    8:required string currency = "";
    9:required string localSymbol = "";
    10:required string tradingClass = "";
    11:required string secIdType = "";
    12:required string secId = "";
    13:required i64 conId = -1;
    14:required double strike = 0.0;

    15:required string execId = "";
    16:required string time = "";
    17:required string acctNumber = "";
    18:required string side = "";
    19:required i32 shares = 0;
    20:required double price = 0.0;
    21:required i32 permId = 0;
    22:required i64 clientId = 0;
    23:required i64 orderId = 0;
    24:required i32 liquidation = 0;
    25:required i32 cumQty = 0;
    26:required double avgPrice = 0.0;
    27:required double evMultiplier = 0.0;
    28:required string orderRef = "";
    29:required string evRule = "";
    30:required string c_currency = "";
    31:required i32 yieldRedemptionDate = 0; // YYYYMMDD format
    32:required double commission = 0.0;
    33:required double realizedPNL = 0.0;
    34:required double gain = 0.0; // yield
}

struct RealTimeBar{
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

struct HistoryRequest{
    1:required string symbol = "";
    2:required string secType = "STK";
    3:required string exchange = "SMART";
    4:required string currency = "USD";
    5:required string primaryExchange = "NASDAQ";
    6:required string endDateTime = "20150824 17:00:00"
    7:required string durationStr = "10 W"
    8:required string barSizeSetting = "1 min"
    9:required string whatToShow = "TRADES"
    10:required i32 useRTH = 1
    11:required i32 formatDate = 1
}

struct Asset{
    1:required i64 conId;
    2:required double strike;
    3:required i32 position;
    4:required double marketPrice;
    5:required double marketValue;
    6:required double averageCost;
    7:required double unrealizedPNL;
    8:required double realizedPNL;

    9:required string accountName;
    10:required string symbol;
    11:required string secType;
    12:required string expiry;
    13:required string right;
    14:required string multiplier;
    15:required string exchange;
    16:required string primaryExchange;
    17:required string currency;
    18:required string localSymbol;
    19:required string tradingClass;
    20:required string secIdType;
    21:required string secId;
}

struct StkPosition{
    1:required string account;
    2:required i32 position;
    3:required double avgCost;
}

struct OptPosition{
    1:required string account;
    2:required i64 conId;
    3:required i32 position;
    4:required double avgCost;
    5:required double strike;

    6:required string symbol;
    7:required string secType;
    8:required string expiry;
    9:required string right;
    10:required string multiplier;
    11:required string exchange;
    12:required string primaryExchange;
    13:required string currency;
    14:required string localSymbol;
    15:required string tradingClass;
    16:required string secIdType;
    17:required string secId;
}

exception Exception{
      1: i32 what;
      2: string why;
}

service Sharp{
    PingResponse ping (1:required PingRequest request);

    i64 getOrderID();
    OrderResponse placeOrder (1:required ContractRequest c_req, 2:required OrderRequest o_req) throws (1:Exception e);
    OrderResponse cancelOrder (1:required i64 o_id) throws (1:Exception e);
    OrderStatus getOrderStatus (1:required i64 o_id) throws (1:Exception e);
    list<OrderStatus> reqOpenOrders() throws (1:Exception e);
    list<OrderStatus> reqAllOpenOrders() throws (1:Exception e);
    void reqGlobalCancel() throws (1:Exception e);
    list<ExecutedContract> reqExecutions(1:required ExecutionFilter ef) throws (1:Exception e);

    void reqRealTimeBars() throws (1:Exception e);
    void addToWatchList(1:required list<string> wl) throws (1:Exception e);
    void removeFromWatchList(1:required list<string> rm) throws (1:Exception e);
    void removeZombieSymbols(1:required list<string> rm) throws (1:Exception e);
    RealTimeBar getNextBar(1:required string symbol) throws (1:Exception e);
    // reqHistoricalData returns the reqId and the name of the file where the data is saved.
    map<i64, string> reqHistoricalData(1:required HistoryRequest request) throws (1:Exception e);
    void cancelHistoricalData(1:required i64 tickerId) throws (1:Exception e);

    map<string,StkPosition> reqStkPositions(1:required bool refresh) throws (1:Exception e);
    map<i64,OptPosition> reqOptPositions(1:required bool refresh) throws (1:Exception e);
    void cancelPositions() throws (1:Exception e);
    map<string, string> reqAccountUpdates(1:required bool subscribe, 2:required string acctCode, 3:required bool refresh) throws (1:Exception e);
    // map<conId, Asset>
    map<i64, Asset> reqPortfolio(1:required bool subscribe, 2:required string acctCode, 3:required bool refresh) throws (1:Exception e);
}

