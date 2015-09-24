import time
import sys, glob
import multiprocessing as mp
from abc import ABCMeta, abstractmethod
sys.path.append('/home/qunzi/sharp/src/py2.7_thrift/')
sys.path.insert(0, glob.glob('/home/qunzi/Downloads/thrift-0.9.2/lib/py/build/lib.linux-x86_64-2.7')[0])

from ib import Sharp
from ib.ttypes import *
from ib.constants import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

import logger
import event
import strategy
import portfolio


logger = logger.createLogger("trader")

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
    def __init__(self, strategy, wl = None, cores = 2, futurelist = True):
        if self._check_values(wl, cores):
            self.wl = wl
            self.cores = cores
        self.futurelist = futurelist
        self._mgr = mp.Manager()
        if self.futurelist: # if promised the possibility to change wl in the future
            self._dict = self._mgr.dict()    # key: the process index; value: list of symbols
        else:
            self._dict = {}
        self._spread_wl()  # spread the symbols as evenly as possible among the worker processes
        self._evnts = mp.Queue()
        self._sts = False   # check if trade() is running.
        self._ps = []       # processes list
        self._stops = [mp.Event() for i in range(cores)] # events for IPC
        self._new_wl_dict = {} # newly being added/removed watchlist which will be distributed to each process
        #if futurelist:      # if promised the possibility to change wl in the future
        self.strategies = [strategy for i in range(cores)]

    @abstractmethod
    def trade(self):
        raise NotImplementedError("Should implement trade().")

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
        if bl:
            self._dict = self._mgr.dict(self._dict)
            self.futurelist = bl
            return True
        else:
            self._dict = dict(self._dict)
            self.futurelist = bl
            return True

    def status(self):    # check if trade() is running.
        return self._sts

    def current_wl(self):# return the current watchlist
        return self.wl

    @abstractmethod
    def _evnts_handler(self, p_id):
        raise NotImplementedError("Should implement _evnts_handler().")

    @abstractmethod
    def _feed_handler(self, p_id): # p_id, process id
        raise NotImplementedError("Should implement _feed_handler().")

    def _spread_new_wl(self, newsbls):
        if not newsbls:
            logger.error("Are you adding empty watchlist?")
            return False
        for s in newsbls:
            if not s.isupper() or len(s) > 4 or len(s) < 1:
                logger.error("adding new wl: watchlist should be uppercase, "
                            "len(symbol) should be less than 5.")
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
        temp = [len(v) for k, v in dict.items()]
        return temp.index(min(temp)) + 1

    def _remove_wl(self, sbls):
        if not sbls:
            logger.error("Are you removing an empty list?")
            return False
        for s in sbls:
            if not s.isupper() or len(s) > 4 or len(s) < 1:
                logger.error("removing wl: watchlist should be uppercase, "
                            "len(symbol) should be less than 5.")
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
        for k, v in dict.items():
            if sbl in v:
                return k

    def _check_values(self, wl, cores):
        if not wl:
            #logger.error("empty watchlist.")
            raise ValueError("empty watchlist.")
        for s in wl:
            if not s.isupper() or len(s) > 4 or len(s) < 1:
                #logger.error("watchlist should be uppercase, len(symbol) should be less than 5.")
                raise ValueError("watchlist should be uppercase, "
                                "len(symbol) should be less than 5.")
        if cores < 2:
            #logger.error("the number of cores should be at least two.")
            raise ValueError("the number of cores should be at least two.")
        if cores-1 > len(wl):
            #logger.error("cores-1 shouldn't be greater than the number of symbols in the watchlist.")
            raise ValueError("cores-1 shouldn't be greater than the number "
                            "of symbols in the watchlist.")
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
    def __init__(self, acctCode, strategy, wl = None, cores = 2, futurelist = True):
        super(LiveTrader, self).__init__(strategy, wl, cores, futurelist)

        self._sockets = [TSocket.TSocket('localhost', 9090) for i in range(cores)]
        self._transports = [TTransport.TBufferedTransport(self._sockets[i]) for i in range(cores)]
        self._protocols = [TBinaryProtocol.TBinaryProtocol(self._transports[i]) for i in range (cores)]
        self._clients = [Sharp.Client(self._protocols[i]) for i in range(cores)]
        for t in self._transports:
            t.open()
        self._clients[0].removeZombieSymbols([])
        self.pfo = portfolio.LivePortfolio(acctCode, self._clients[0])
        self.open_odrs = self._mgr.dict()
        self.fill_odrs = self._mgr.dict()
        self._odr_lk = mp.Lock()

    def trade(self):
        if self._sts:
            logger.error("trade() is already running.")
            return
        for stp in self._stops:
            stp.clear()
        logger.warn("trade() starts running ...")
        for i in range(self.cores):
            if i == 0:
                # process 0 only handles the queue self._evnts
                self._pre_trade()
                worker = mp.Process(target = self._evnts_handler, args = (i,))
                self._ps.append(worker)
                self._ps[-1].daemon = True
                self._ps[-1].start()
            else:
                # each worker process handles the bar feed and
                # send decisions to self._evnts
                worker = mp.Process(target = self._feed_handler, args = (i,))
                self._ps.append(worker)
                self._ps[-1].daemon = True
                self._ps[-1].start()
        self._sts = True

    def addToWatchList(self, symbols):
        if self.futurelist and self._sts:
            if self._spread_new_wl(symbols):
                with self._odr_lk:
                    for k, v in self._new_wl_dict.items():
                        old_list = self._dict[k]
                        old_list.extend(v)
                        self._dict[k] = old_list
                        self._clients[0].addToWatchList(v)
                return True
            else:
                return False
        else:
            logger.error("futurelist = False, you have promised not "
                        "to add new symbols; or trade() is not running.")
            return False

    def removeFromWatchList(self, symbols):
        if self.futurelist and self._sts:
            if self._remove_wl(symbols):
                with self._odr_lk:
                    self._client.removeFromWatchList(self.wl)
                for k, v in self._new_wl_dict.items():
                    old_list = self._dict[k]
                    for item in v:
                        old_list.remove(item)
                    self._dict[k] = old_list
                return True
            else:
                return False
        else:
            logger.error("futurelist = False, you have promised not to "
                        "remove new symbols; or trade() is not running.")
            return False

    def _pre_trade(self):
        self._clients[0].addToWatchList(self.wl)

    def _post_trade(self):
        self._clients[0].removeFromWatchList([])

    def get_open_odrs(self):
        #with self._odr_lk:
        return dict(self.open_odrs)

    def terminate(self):
        if self._sts:
            logger.warn("stop() must be called first to terminate.")
            return False
        for t in self._transports:
            t.close()
        return True

    def reqGlobalCancel(self): # better call this function when trade() is not running.
        logger.warn("calling reqGlobalCancel: cancel all open orders")
        with self._odr_lk:
            self._clients[0].reqGlobalCancel()
            self.open_odrs.clear()
        return True

    def _evnts_handler(self, p_id):
        prev_time = time.time()
        while (not self._stops[p_id].is_set()):
            filled_flag = False
            try:
                evnt = self._evnts.get()
            except mp.Queue.Empty:
                pass
            else:
                if evnt.type == 'SIGNAL':
                    co = self.strategies[p_id].generate_order(evnt, self.pfo)
                    if co is not None:
                        # placing order
                        logger.info("Placing order, symbol = %s, action = %s, quantity = %d, "
                                    "price = %f, order_type = %s",
                                    co._c.symbol, co._o.action, co._o.totalQuantity,
                                    co._o.lmtPrice, co._o.orderType)
                        with (not self.futurelist) or self._odr_lk:
                            o_resp = self._clients[p_id].placeOrder(co._c, co._o)
                        logger.info("o_resp.orderId = %d", o_resp.orderId)
                        self.open_odrs[o_resp.orderId] = o_resp
            finally:
                # when self.open_odrs is not empty, check if some order is filled
                # every 300 seconds (i.e. 5 minutes)
                if int(time.time()-prev_time) > 300:
                    for i, v in self.open_odrs.items():
                        with (not self.futurelist) or self._odr_lk:
                            o_stus = self._clients[p_id].getOrderStatus(i)
                        self.open_odrs[i] = o_stus
                        if o_stus.status == 'Filled':
                            filled_flag = True
                            self.fill_odrs[i] = o_stus
                            self.open_odrs.pop(i)
                            logger.info("Order %d is filled, symbol = %s, "
                                        "action = %s, quantity = %d, "
                                        "avgFillPrice = %f, lastFillPrice = %f",
                                        o_stus.orderId, o_stus.symbol,
                                        o_stus.action, o_stus.totalQuantity,
                                        o_stus.avgFillPrice, o_stus.lastFillPrice)
                    if filled_flag:
                        self.pfo.update_fill()
                    prev_time = time.time()

    def _feed_handler(self, p_id):
        while not self._stops[p_id].is_set():
            #with (not self.futurelist) or self._locks[p_id]:
            for s in self._dict[p_id]:
                feed = self._clients[p_id].getNextBar(s)
                signal = self.strategies[p_id].generate_signal(feed)
                if signal is not None:
                    self._evnts.put(signal)

class TestTrader(BaseTrader):
    """docstring for TestTrader"""
    def __init__(self, strategy, wl = None, cores = 2, futurelist = True):
        super(TestTrader, self).__init__(strategy, wl, cores, futurelist)

'''
if __name__ == '__main__':

    watchlist = ['AAPL', 'AMZN']
    acct = 'DU224610'
    naive_strategy = strategy.NaiveStrategy()
    trader = LiveTrader(acct, naive_strategy, watchlist)
'''