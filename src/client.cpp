#include "sharp.h"
#include <thread>

using namespace sharp;

int main(int argc, char const *argv[])
{
    std::unique_ptr<SharpClientService> trader(make_client());

    Contract contract;
    Order order;

    contract.symbol = "BIDU";
    contract.secType = "STK";
    contract.exchange = "SMART";
    contract.currency = "USD";

    order.action = "BUY";
    order.totalQuantity = 1000;
    order.orderType = "LMT";
    order.lmtPrice = 0.09;

    std::cout<<"***"<<trader->getOrderID()<<std::endl;

    OrderResponse response;

    trader->placeOrder(response, contract, order);

    int64_t o_id = response.orderId;

    std::this_thread::sleep_for (std::chrono::seconds(20));

    trader->cancelOrder(response, o_id);

    std::cout<<"response.state = "<<response.state<<std::endl;

    return 0;
}
