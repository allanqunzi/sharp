# event.py

class Event(object):
    """
    """
    pass

class OrderEvent(Event):
    """
    User defined strategy takes real time bars as input and outputs
    OrderEvent to the event queue which trader._evnts_handler is
    handling.
    """

    def __init__(self, symbol, direction, quantity, order_type, price):
        """
        Initialises the order type, setting whether it is
        a Market order ('MKT') or Limit order ('LMT'), has
        a quantity (integral) and its direction ('BUY' or
        'SELL').

        Parameters:
        symbol - The instrument to trade, e.g. 'AAPL'
        direction - 'BUY' or 'SELL' for long or short
        quantity - Non-negative integer for quantity
        order_type - 'MKT' or 'LMT' for Market or Limit
        price - the price limit for 'LMT' order_type
        """
        self._cr = ContractRequest(symbol, 'STK', 'SMART', 'USD')
        self._or = OrderRequest(direction, quantity, order_type, price)

class FillEvent(Event):
    """
    Encapsulates the notion of a Filled Order, as returned
    from a brokerage. Stores the quantity of an instrument
    actually filled and at what price. In addition, stores
    the commission of the trade from the brokerage.
    """

    def __init__(self, timeindex, symbol, exchange, quantity,
                 direction, fill_cost, commission=None):
        """
        Initialises the FillEvent object. Sets the symbol, exchange,
        quantity, direction, cost of fill and an optional
        commission.

        If commission is not provided, the Fill object will
        calculate it based on the trade size and Interactive
        Brokers fees.

        Parameters:
        timeindex - The bar-resolution when the order was filled.
        symbol - The instrument which was filled.
        exchange - The exchange where the order was filled.
        quantity - The filled quantity.
        direction - The direction of fill ('BUY' or 'SELL')
        fill_cost - The holdings value in dollars.
        commission - An optional commission sent from IB.
        """