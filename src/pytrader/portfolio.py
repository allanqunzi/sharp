import time
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
        Updates the portfolio from a FillEvent.
        """
        raise NotImplementedError("Should implement update_fill()")

class LivePortfolio(Portfolio):
    """
    docstring for LivePortfolio
    """

    def __init__(self, acctCode, thrift_client):
        #super(LivePortfolio, self).__init__()
        if acctCode:
            self.acctCode = acctCode
        else:
            raise ValueError("acctCode shouldn't be empty.")

        self._client = thrift_client
        self._accnt_updates = None

        self.CashBalance = 0.0
        self.StockMarketValue = 0.0
        self.OptionMarketValue = 0.0
        self.LongOptionValue = 0.0
        self.ShortOptionValue = 0.0
        self.DayTradesRemaining = 0
        self.timeStamp = ''
        self._construct_account()

        # self.current_assets is a dict, key-symbol or timeStamp; value-Asset or timeStamp
        self.current_assets = self._construct_current_assets()
        self.all_assets = self._construct_all_assets() # list of dicts

    def _construct_account(self):
        self._accnt_updates = self._client.reqAccountUpdates(True, self.acctCode, True)
        if (not self._accnt_updates) or self._accnt_updates["acctCode"] != self.acctCode:
            raise ValueError("reqAccountUpdates failed in LivePortfolio._client.reqAccountUpdates.")

        self.CashBalance = float(self._accnt_updates["CashBalance"].split(':')[1])
        self.StockMarketValue = float(self._accnt_updates["StockMarketValue"].split(':')[1])
        self.OptionMarketValue = float(self._accnt_updates["OptionMarketValue"].split(':')[1])
        #self.LongOptionValue = float(self._accnt_updates["LongOptionValue"].split(':')[1])
        #self.ShortOptionValue = float(self._accnt_updates["ShortOptionValue"].split(':')[1])
        self.DayTradesRemaining = int(self._accnt_updates["DayTradesRemaining"].split(':')[1])
        self.timeStamp = time.strftime('%Y%m%d') + self._accnt_updates["timeStamp"]

        # cancel the subscription
        self._client.reqAccountUpdates(False, self.acctCode, True)

    def _construct_current_assets(self, refresh = False): # only stock assets
        asts = self._client.reqPortfolio(True, self.acctCode, refresh)
        d = {}
        if asts:
            for k, v in asts.items():
                if v.accountName != self.acctCode:
                    raise ValueError("The returned acctCode from self._client.reqPortfolio is not the same as self.acctCode.")
                if v.symbol not in d:
                    d[v.symbol] = v
                else:
                    raise ValueError("An identical symbol reoccurs in assets.")
            d["timeStamp"] = self.timeStamp
        return d

    def _construct_all_assets(self):
        return [self.current_assets]

    def update_fill(self):
        try:
            self._construct_account()
        except ValueError, v:
            logger.error(v)
        else:
            self.all_assets.append(self.current_assets)
            self.current_assets = self._construct_current_assets()













