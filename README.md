# sharp

> An automated, algorithmic and customizable trading system for Interactive Brokers.


Author:  Aiqun Huang, aiqunhuang@knights.ucf.edu

sharp is an automated trading system developed in connection with the C++ API provided by Interactive Brokers (IB), it has
an easy-to-use Python interface, by which users can write their own strategies, explore and apply their own creative ideas
both in the simulated and real live trading enviroments. With sharp, you can trade easily, interactively and algorithmically.

sharp has two major components. The first one is interfacing with the IB Trader Workstation (TWS) and it is written in modern 
C++ (C++11) and has used the Boost library and Apache Thrift framework to implement a multithreaded server; the second component
is the client talking to the thrift server and written in Python 2.7, the python code has multiprocessing support to monitor and trade multiple stocks at the same time, and provides an easy and customizable interface for the users of sharp.

### Demo
Create a trader and initialize it with a watchlist, the account ID, and a trading strategy.
```python
>>> import trader
>>> import strategy
>>> watchlist = ['AAPL', 'AMZN']
>>> account = 'DU218612'
>>> naive_strategy = strategy.NaiveStrategy()
>>> ib_trader = trader.LiveTrader(account, naive_strategy, watchlist)
```
Start trading, monitor the market feed for watchlist, and place orders based on the strategy provided
```python
>>> ib_trader.trade()
```
Add new symbols to the watchlist, orders will be placed for these symbols according to the strategy provided earlier
```python
>>> ib_trader.addToWatchList(['BABA', 'NFLX'])
>>> True # symbols added to the watchlist successfully
```
Remove symbols from the watchlist and stop trading for these symbols
```python
>>> ib_trader.removeFromWatchList(['AMZN'])
>>> True # symbols removed from the watchlist successfully
```
Get the current watchlist, which is a python list
```python
>>> wl = ib_trader.current_wl()
```
Get the account cash balance, which returns a two-element list, the first element is the balance (a number),
the second is the time stamp for the balance value.
```python
>>> balance = ib_trader.get_accnt_balance()
```
Get the current open orders, the returned is a python dictionary, 
key is `orderId`, value is an `OrderStatus` class instance.
```python
>>> open_orders = ib_trader.get_open_odrs()
```
Get the filled orders, the returned is a python dictionary, 
keys are `orderId`s, values are instances of the class `OrderStatus`.
```python
>>> fill_orders = ib_trader.get_fill_odrs()
```
Stop trading
```python
>>> ib_trader.stop()
>>> True # trading stops successfully
```
Cancel all open orders
```python
>>> ib_trader.reqGlobalCancel()
>>> True # all open orders canceled successfully
```
Terminate the trader, cutting off the connection to the IB TWS
```python
>>> ib_trader.terminate()
>>> True # trader terminated successfully
```


