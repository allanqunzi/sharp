# data.py

import logger
import pandas as pd

from abc import ABCMeta, abstractmethod

logger = logger.createLogger("data_")

class DataHandler(object):
    """
    DataHandler is an abstract base class providing an interface for
    all subsequent (inherited) data handlers (both live and historic).

    The goal of a (derived) DataHandler object is to output a generated
    bar (datetime, open, low, high, close, volume, barCount, WAP, hasGaps)
    for the symbol requested.

    This will replicate how a live strategy would function as current
    market data would be sent "down the pipe". Thus a historic and live
    system will be treated identically by the rest of the backtesting suite.
    """

    __metaclass__ = ABCMeta

    @abstractmethod
    def get_latest_bar(self, symbol):
        """
        Returns the latest bar for symbol, which is a tuple
        (datetime, open, low, high, close, volume, barCount, WAP, hasGaps)
        """
        raise NotImplementedError("Should implement get_latest_bar()")

class HistoricCSVDataHandler(DataHandler):
    """
    HistoricCSVDataHandler is designed to read CSV files for
    the requested symbol from disk and provide an interface
    to obtain the "latest" bar in a manner identical to a live
    trading interface.
    """

    def __init__(self, csv_dir, symbol):
        """
        Initialises the historic data handler by
        the full path of the CSV file and the symbol.

        Parameters:
        csv_dir - Absolute directory path to the CSV file.
        symbol - The symbol.
        """
        self.csv_dir = csv_dir
        self.symbol = symbol

        self.symbol_data = pd.DataFrame()
        self.continue_backtest = True
        self._open_convert_csv_files()
        self.generator = self._get_new_bar()

    def _open_convert_csv_files(self):
        """
        Opens the CSV file from the data directory, converting
        it into a pandas DataFrame as self.symbol_data.

        For this handler it will be assumed that the data was generated
        from function EWrapperImpl::historicalData. Thus its format will
        be respected.
        """
        # Load the CSV file with no header information, indexed on 0,1,2,...
        self.symbol_data = pd.read_csv(
                                  self.csv_dir,
                                  header=None, index_col=None,
                                  names=['datetime','open','low','high','close',
                                        'volume', 'barCount', 'WAP', 'hasGaps'
                                        ]
                              )

    def _get_new_bar(self):
        """
        Returns the latest bar from the data feed as a tuple of
        (datetime, open, low, high, close, volume, barCount, WAP, hasGaps).
        """
        for t in self.symbol_data.itertuples():
            yield t

    def get_latest_bar(self):
        """
        Returns the latest bar, which is a tuple
        """
        try:
            bar = self.generator.next()
        except StopIteration:
            logger.warn('StopIteration for get_latest_bar.')
            self.continue_backtest = False
        else:
            return bar

class LiveIBDataHandler(DataHandler):
    """
    LiveIBDataHandler is designed to wrap the client.getNextBar
    called on the thrift client side
    """

    def __init__(self, client, symbol):
        """
        Initialises the live data handler by an instance of the
        thrift client and the symbol.

        Parameters:
        client - An instance of thrift client.
        symbol - The symbol.
        """
        self.client = client
        self.symbol = symbol

    def get_latest_bar(self):
        """
        Returns the latest bar, which is a tuple
        """
        try:
            bar = self.client.getNextBar(symbol)
        except RuntimeError:
            logger.error('error emitted from getNextBar.')
        else:
            return bar


if __name__ == "__main__":
    hist = HistoricCSVDataHandler("aapl.csv", "AAPL")
    lb = hist.get_latest_bar()
    print(lb)
    lb = hist.get_latest_bar()
    print(lb)
    lb = hist.get_latest_bar()
    print(lb)