from Queue import Queue
import threading
#from threading import Thread # this way Thread can be directly used without "threading."
import sys, glob
import time
sys.path.append('/home/qunzi/sharp/src/py2.7_thrift/')
sys.path.insert(0, glob.glob('/home/qunzi/Downloads/thrift-0.9.2/lib/py/build/lib.linux-x86_64-2.7')[0])

#sys.path.append('/home/qunzi/Projects/IBJts_test/sample_TestPosix/py3.4_thrift/')
#sys.path.insert(0, glob.glob('/home/qunzi/Downloads/thrift-0.9.2/lib/py/build/lib.linux-x86_64-3.4')[0])

from ib import Sharp
from ib.ttypes import *
from ib.constants import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
#from thrift.protocol import TCompactProtocol

#try:
# Make socket
socket = TSocket.TSocket('localhost', 9090)

# Buffering is critical. Raw sockets are very slow
transport = TTransport.TBufferedTransport(socket)

# Wrap in a protocol
protocol = TBinaryProtocol.TBinaryProtocol(transport)
#protocol = TCompactProtocol.TCompactProtocol(transport)

# Create a client to use the protocol encoder
client = Sharp.Client(protocol)

# Connect!
transport.open()


ping_req = PingRequest()
ping_resp = client.ping(ping_req)
print('ping....')

o_id = client.getOrderID()
print(o_id)


def getNewBar(q, sbl, stop_event):
  while not stop_event.isSet():
    q.put(client.getNextBar(sbl), False)
    print(sbl)
    print(sbl, "q.qsize() = ", q.qsize())
    print(q.get(False).open)

stop_events = []
stop_events.append(threading.Event())
stop_events.append(threading.Event())


def monitor(dict, list, threads):
  dict.clear() # remove all entries in dict
  for l in list:
    dict[l] = Queue()
  client.addToWatchList(list)
  num_threads = len(list)
  for i in range(num_threads):
    symbol = list[i]
    worker = threading.Thread(target=getNewBar, args=(dict[symbol], symbol, stop_events[i]))
    threads.append(worker)
    worker.setDaemon(True)
    worker.start()

c_req = ContractRequest('V', 'STK', 'SMART', 'USD')
o_req = OrderRequest('BUY', 1000, 'LMT', 0.12)
place_resp = client.placeOrder(c_req, o_req)
o_id1 = place_resp.orderId
print('place_resp.state =', place_resp.state, o_id1)


c_req = ContractRequest('GOOG', 'STK', 'SMART', 'USD')
o_req = OrderRequest('BUY', 500, 'LMT', 0.15)
place_resp = client.placeOrder(c_req, o_req)
o_id2 = place_resp.orderId
print('place_resp.state =', place_resp.state, o_id2)

os = client.getOrderStatus(o_id1)
print("os.symbol =", os.symbol, "os.status", os.status)

os = client.getOrderStatus(o_id2)
print("os.symbol =", os.symbol, "os.status", os.status)

time.sleep(250)




'''
opens = client.reqOpenOrders()

for o in opens:
  print(o.orderId, "---", o.symbol, "---", o.status)

client.reqGlobalCancel()

time.sleep(160)

o_id = place_resp.orderId
print('o_resp.orderId = ', o_id)
cancel_resp = client.cancelOrder(o_id)

print('cancel_resp.state =', cancel_resp.state)


'''
qdict = {}
wlist = ["AAPL", "GOOG"]
ts = []
client.removeZombieSymbols([])

hr = HistoryRequest();
hr.symbol = "AAPL"
hr.useRTH = 0
hr.primaryExchange = "NYSE"
hr.durationStr = "800 S";
hr.barSizeSetting = "5 secs"
hr.endDateTime = "20150804 10:10:45"

m = client.reqHistoricalData(hr)

for (k, v) in m.iteritems():
  print(k, "---", v)


'''
acnt = client.reqAccountUpdates(True, "DU224610", True)

for (k, v) in acnt.iteritems():
  print(k, "---", v)
'''
time.sleep(300)

client.addToWatchList(wlist)

while True:
  res = client.getNextBar(wlist[0])
  print(wlist[0], "open = ", res.open)
  res = client.getNextBar(wlist[1])
  print(wlist[1], "open = ", res.open)


#monitor(qdict, wlist, ts)


time.sleep(20)
for i in range(len(stop_events)):
  stop_events[i].set()
  ts[i].join()

client.removeFromWatchList(wlist)

time.sleep(10)

print("done")

#while threading.active_count() > 0: # this while responds to ctrl + c
#  time.sleep(0.1)

'''
q = Queue()
b1 = client.addToWatchList(["AMZN"])
if b1:
  print('watch list added.')

while True:
  q.put(client.getNextBar("AMZN"))
  print(q.get().open)
'''


transport.close()

#except Exception, e:
#except Exception as e:
# print(e.what, e.why)






