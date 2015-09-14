import datetime
import numpy as np
import pandas as pd
from abc import ABCMeta, abstractmethod

class Portfolio(object):
    """
    """
    __metaclass__ = ABCMeta

    @abstractmethod
    def update_fill(self, event):
        """
        Updates the portfolio current positions and holdings
        from a FillEvent.
        """
        raise NotImplementedError("Should implement update_fill()")

class LivePortfolio(Portfolio):
    """
    docstring for LivePortfolio
    """

    def __init__(self, acctCode, thrift_client):
        #super(LivePortfolio, self).__init__()
        self._client = thrift_client
        self._accnt_updates = self._client.reqAccountUpdates(True, acctCode, True)
        if (not self._accnt_updates) or self._accnt_updates["acctCode"] != acctCode:
            raise ValueError("reqAccountUpdates failed in LivePortfolio initialization.")
        self.acctCode = acctCode
        self.CashBalance = float(self._accnt_updates["CashBalance"].split(':')[1])
        self.StockMarketValue = float(self._accnt_updates["StockMarketValue"].split(':')[1])
        self.OptionMarketValue = float(self._accnt_updates["OptionMarketValue"].split(':')[1])
        self.LongOptionValue = float(self._accnt_updates["LongOptionValue"].split(':')[1])
        self.ShortOptionValue = float(self._accnt_updates["ShortOptionValue"].split(':')[1])
        self.DayTradesRemaining = int(self._accnt_updates["DayTradesRemaining"].split(':')[1])






