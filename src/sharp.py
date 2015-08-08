
import sys, glob
import time
sys.path.append('/home/qunzi/Projects/IBJts_test/sample_TestPosix/py2.7_thrift/')
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

try:
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

  c_req = ContractRequest('AMZN', 'STK', 'SMART', 'USD')
  o_req = OrderRequest('BUY', 1000, 'LMT', 0.12)

  place_resp = client.placeOrder(c_req, o_req)
  print('place_resp.state =', place_resp.state)

  time.sleep(20)

  o_id = place_resp.orderId
  print('o_resp.orderId = ', o_id)
  cancel_resp = client.cancelOrder(o_id)

  print('cancel_resp.state =', cancel_resp.state)

  transport.close()

except Exception, e:
#except Exception as e:
  print(e.what, e.why)
