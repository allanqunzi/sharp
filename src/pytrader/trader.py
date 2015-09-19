import time
import sys, glob
import multiprocessing as mp
from abc import ABCMeta, abstractmethod
sys.path.append('/home/qunzi/sharp/src/py2.7_thrift/')
sys.path.insert(0, glob.glob('/home/qunzi/Downloads/thrift-0.9.2/lib/py/build/lib.linux-x86_64-2.7')[0])

import logger

from ib import Sharp
from ib.ttypes import *
from ib.constants import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol


logger = logger.createLogger("trader_")

class AbstractTrader(object):
    """
    the interface
    """
    __metaclass__ = ABCMeta

    @abstractmethod
    def trade(self):
        """
        monitor the data feed for watchlist and trade necessarily
        """
        raise NotImplementedError("Should implement trade()")

    @abstractmethod
    def stop(self):
        """
        stop self.trade()
        """
        raise NotImplementedError("Should implement stop()")

    @abstractmethod
    def status(self):
        """
        check if self.trade() is running
        """
        raise NotImplementedError("Should implement status()")

    @abstractmethod
    def current_wl(self):
        """
        return the current watchlist
        """
        raise NotImplementedError("Should implement current_wl()")

    @abstractmethod
    def addToWatchList(self, symbols):
        """
        add symbols to watchlist
        """
        raise NotImplementedError("Should implement addToWatchList()")

    @abstractmethod
    def removeFromWatchList(self, symbols):
        """
        remove symbols from watchlist
        """
        raise NotImplementedError("Should implement removeFromWatchList()")

class BaseTrader(AbstractTrader):
    """
    initialize BaseTrader with watchlist, number of cores,
    the number of cores is the number of processes

    process 0 - the process handling the order events
    cores-1 - the number of worker processes for handling the bars
    """
    def __init__(self, wl = [], cores = 2, futurelist = True, strategy):
        if self._check_values(wl, cores):
            self.wl = wl
            self.cores = cores
        self._dict = {}    # key: the process index; value: list of symbols
        self._spread_wl()  # spread the symbols as evenly as possible among the worker processes
        self._evnts = mp.Queue()
        self._sts = False   # check if trade() is running.
        self._ps = []       # processes list
        self._stops = [mp.Event() for i in range(cores)] # events for IPC
        self._new_wl_dict = {} # newly being added/removed watchlist which will be distributed to each process
        #if futurelist:      # if promised the possibility to change wl in the future
        self._locks = [mp.Lock() for i in range(cores)]
        self.strategy = strategy

    def trade(self):
        if self._sts:
            logger.error("trade() is already running.")
            return
        for i in range(cores):
            if i == 0:
                # process 0 only handles the queue self._evnts
                self._pre_trade()
                worker = mp.Process(target = self._evnts_handler, args = ())
                self._ps.append(worker)
                worker.daemon = True
                worker.start()
            else:
                # each worker process handles the bar feed and
                # send decisions to self._evnts
                worker = mp.Process(target = self._feed_handler, args = ((self._dict[i]),))
                self._ps.append(worker)
                worker.daemon = True
                worker.start()
        self._sts = True

    def stop(self):
        if not self._sts:
            logger.warn("trade() is not running, not meaningful to stop.")
            return True
        else:
            for i in range(self.cores):
                self._stops[i].set()
                self._ps[i].join()
            self._post_trade()
            self._sts = False
            logger.info("trade() has stopped running.")
        return True

    @abstractmethod
    def _pre_trade(self):
        raise NotImplementedError("Should implement _pre_trade().")

    @abstractmethod
    def _post_trade(self):
        raise NotImplementedError("Should implement _post_trade().")

    def set_futurelist(self, bl):
        if self._sts:
            logger.error("futurelist can not be set while trade() is running.")
            return False
        elif self.futurelist == bl:
            logger.warn("futurelist is already", bl)
            return True
        else:
            logger.info("futurelist is set to be", bl)
            self.futurelist = bl;
            return True

    def status(self):    # check if trade() is running.
        return self._sts

    def current_wl(self):# return the current watchlist
        return self.wl

    @abstractmethod
    def _evnts_handler(self):
        raise NotImplementedError("Should implement _evnts_handler().")

    @abstractmethod
    def _feed_handler(self):
        raise NotImplementedError("Should implement _feed_handler().")

    def _spread_new_wl(self, newsbls):
        if not newsbls:
            logger.error("Are you adding empty watchlist?")
            return False
        for s in newsbls:
            if not s.isupper() or len(s) > 4 or len(s) < 1:
                logger.error("adding new wl: watchlist should be uppercase, len(symbol) should be less than 5.")
                return False
        dcopy = self._dict.copy()
        wcopy = list(self.wl)
        self._new_wl_dict.clear()
        for s in newsbls:
            if s not in wcopy:
                hit = self._min_p(dcopy)
                dcopy[hit].append(s)
                wcopy.append(s)
                if hit in self._new_wl_dict:
                    self._new_wl_dict[hit].append(s)
                else:
                    self._new_wl_dict[hit] = [s]
        self.wl = wcopy
        return True

    def _min_p(self, dict): # return the process id which has least symbols
        temp = [len(v) for k, v in dict]
        return temp.index(min(temp)) + 1

    def _remove_wl(self, sbls):
        if not sbls:
            logger.error("Are you removing an empty list?")
            return False
        for s in sbls:
            if not s.isupper() or len(s) > 4 or len(s) < 1:
                logger.error("removing wl: watchlist should be uppercase, len(symbol) should be less than 5.")
                return False
        dcopy = self._dict.copy()
        wcopy = list(self.wl)
        self._new_wl_dict.clear()
        for s in sbls:
            if s in wcopy:
                hit = self._locate_p(s, dcopy)
                dcopy[hit].remove(s)
                wcopy.remove(s)
                if hit in self._new_wl_dict:
                    self._new_wl_dict[hit].append(s)
                else:
                    self._new_wl_dict[hit] = [s]
        self.wl = wcopy
        return True

    def _locate_p(self, sbl, dict): # return the process id which has sbl
        for k, v in dict:
            if sbl in v:
                return k

    def _check_values(self, wl, cores):
        if not wl:
            #logger.error("empty watchlist.")
            raise ValueError("empty watchlist.")
        for s in wl:
            if not s.isupper() or len(s) > 4 or len(s) < 1:
                #logger.error("watchlist should be uppercase, len(symbol) should be less than 5.")
                raise ValueError("watchlist should be uppercase, len(symbol) should be less than 5.")
        if cores < 2:
            #logger.error("the number of cores should be at least two.")
            raise ValueError("the number of cores should be at least two.")
        if cores-1 > len(wl):
            #logger.error("cores-1 shouldn't be greater than the number of symbols in the watchlist.")
            raise ValueError("cores-1 shouldn't be greater than the number of symbols in the watchlist.")
        return True

    def _spread_wl(self):
        num_p = (self.cores - 1) # number of processes
        num_s = len(self.wl)     # number of symbols
        a = num_s/num_p
        b = num_s%num_p
        cur = 0
        for i in range(num_p):
            self._dict[i+1] = [self.wl[cur]]
            cur += 1
            for j in range(cur, cur+a-1):
                self._dict[i+1].append(self.wl[j])
            cur += a-1
            if b > 0:
                self._dict[i+1].append(self.wl[cur])
                b -= 1
                cur += 1
        assert num_s == cur, "error in _spread_wl."



class LiveTrader(BaseTrader):
    """
    initialize LiveTrader with watchlist, number of cores and
    thrift client, and make connection to thrift server.
    """
    def __init__(self, wl=[], cores = 2, futurelist = True, strategy):
        super(LiveTrader, self).__init__(wl, cores, futurelist, strategy)

        self._socket = TSocket.TSocket('localhost', 9090)
        self._transport = TTransport.TBufferedTransport(self._socket)
        self._protocol = TBinaryProtocol.TBinaryProtocol(self._transport)
        self._client = Sharp.Client(self._protocol)
        #self._transport.open()
        #self._client.removeZombieSymbols([])

    def addToWatchList(self, symbols):
        if self.futurelist and self._sts:
            if self._spread_new_wl(symbols):
                for k, v in self._new_wl_dict:
                    with self._locks[k]:
                        self._dict[k].extend(v)
                        self._client.addToWatchList(v)
                return True
            else:
                return False
        else:
            logger.error("futurelist = False, you have promised not to add new symbols; or trade() is not running.")
            return False

    def removeFromWatchList(self, symbols):
        if self.futurelist and self._sts:
            if self._remove_wl(symbols):
                self._client.removeFromWatchList(self.wl)
                for k, v in self._new_wl_dict:
                    with self._locks[k]:
                        for item in v:
                            self._dict[k].remove(item)
                return True
            else:
                return False
        else:
            logger.error("futurelist = False, you have promised not to remove new symbols; or trade() is not running.")
            return False

    def _pre_trade(self):
        pass

    def _post_trade(self):
        pass

    def _evnts_handler(self):
        pass

    def _feed_handler(self):
        pass

class TestTrader(BaseTrader):
    """docstring for TestTrader"""
    def __init__(self, wl=[], cores = 2, futurelist = True):
        super(TestTrader, self).__init__(wl, cores, futurelist)


if __name__ == '__main__':

    watchlist = ['AAPL', 'AMZN']
    trader = LiveTrader(watchlist)