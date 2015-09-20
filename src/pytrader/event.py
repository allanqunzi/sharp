# event.py

from ib import Sharp
from ib.ttypes import *
from ib.constants import *

class Event(object):
    """
    """
    pass

class SignalEvent(Event):
    """
    User defined strategy takes real time bars as input and outputs
    SignalEvent(if necessary) to the event queue which trader._evnts_handler
    is handling.
    """

    def __init__(self, symbol, datetime, signal_type, strength):
        """
        Initialises the SignalEvent.

        Parameters:
        symbol - The ticker symbol, e.g. 'GOOG'.
        datetime - The timestamp at which the signal was generated.
        signal_type - 1(LONG) or 0(SHORT).
        strength - a float number in the range of [0, 10] or user defined
        """

        self.type = 'SIGNAL'
        self.symbol = symbol
        self.datetime = datetime
        self.signal_type = signal_type
        self.strength = strength

class OrderEvent(Event):
    """
    trader._evnts_handler handles the SignalEvent poped from the event queue,
    and takes self.portfolio as input, and generates OrderEvent or None.
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
        self.type = 'ORDER'
        self._c = ContractRequest(symbol, 'STK', 'SMART', 'USD')
        self._o = OrderRequest(direction, quantity, order_type, price)
