#
# Autogenerated by Thrift Compiler (0.9.2)
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#
#  options string: py
#

from thrift.Thrift import TType, TMessageType, TException, TApplicationException

from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol, TProtocol
try:
  from thrift.protocol import fastbinary
except:
  fastbinary = None



class PingRequest:

  thrift_spec = (
  )

  def read(self, iprot):
    if iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastbinary is not None:
      fastbinary.decode_binary(self, iprot.trans, (self.__class__, self.thrift_spec))
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and self.thrift_spec is not None and fastbinary is not None:
      oprot.trans.write(fastbinary.encode_binary(self, (self.__class__, self.thrift_spec)))
      return
    oprot.writeStructBegin('PingRequest')
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def validate(self):
    return


  def __hash__(self):
    value = 17
    return value

  def __repr__(self):
    L = ['%s=%r' % (key, value)
      for key, value in self.__dict__.iteritems()]
    return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

  def __eq__(self, other):
    return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

class PingResponse:

  thrift_spec = (
  )

  def read(self, iprot):
    if iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastbinary is not None:
      fastbinary.decode_binary(self, iprot.trans, (self.__class__, self.thrift_spec))
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and self.thrift_spec is not None and fastbinary is not None:
      oprot.trans.write(fastbinary.encode_binary(self, (self.__class__, self.thrift_spec)))
      return
    oprot.writeStructBegin('PingResponse')
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def validate(self):
    return


  def __hash__(self):
    value = 17
    return value

  def __repr__(self):
    L = ['%s=%r' % (key, value)
      for key, value in self.__dict__.iteritems()]
    return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

  def __eq__(self, other):
    return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

class ContractRequest:
  """
  Attributes:
   - symbol
   - secType
   - exchange
   - currency
  """

  thrift_spec = (
    None, # 0
    (1, TType.STRING, 'symbol', None, None, ), # 1
    (2, TType.STRING, 'secType', None, None, ), # 2
    (3, TType.STRING, 'exchange', None, None, ), # 3
    (4, TType.STRING, 'currency', None, None, ), # 4
  )

  def __init__(self, symbol=None, secType=None, exchange=None, currency=None,):
    self.symbol = symbol
    self.secType = secType
    self.exchange = exchange
    self.currency = currency

  def read(self, iprot):
    if iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastbinary is not None:
      fastbinary.decode_binary(self, iprot.trans, (self.__class__, self.thrift_spec))
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      if fid == 1:
        if ftype == TType.STRING:
          self.symbol = iprot.readString();
        else:
          iprot.skip(ftype)
      elif fid == 2:
        if ftype == TType.STRING:
          self.secType = iprot.readString();
        else:
          iprot.skip(ftype)
      elif fid == 3:
        if ftype == TType.STRING:
          self.exchange = iprot.readString();
        else:
          iprot.skip(ftype)
      elif fid == 4:
        if ftype == TType.STRING:
          self.currency = iprot.readString();
        else:
          iprot.skip(ftype)
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and self.thrift_spec is not None and fastbinary is not None:
      oprot.trans.write(fastbinary.encode_binary(self, (self.__class__, self.thrift_spec)))
      return
    oprot.writeStructBegin('ContractRequest')
    if self.symbol is not None:
      oprot.writeFieldBegin('symbol', TType.STRING, 1)
      oprot.writeString(self.symbol)
      oprot.writeFieldEnd()
    if self.secType is not None:
      oprot.writeFieldBegin('secType', TType.STRING, 2)
      oprot.writeString(self.secType)
      oprot.writeFieldEnd()
    if self.exchange is not None:
      oprot.writeFieldBegin('exchange', TType.STRING, 3)
      oprot.writeString(self.exchange)
      oprot.writeFieldEnd()
    if self.currency is not None:
      oprot.writeFieldBegin('currency', TType.STRING, 4)
      oprot.writeString(self.currency)
      oprot.writeFieldEnd()
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def validate(self):
    if self.symbol is None:
      raise TProtocol.TProtocolException(message='Required field symbol is unset!')
    if self.secType is None:
      raise TProtocol.TProtocolException(message='Required field secType is unset!')
    if self.exchange is None:
      raise TProtocol.TProtocolException(message='Required field exchange is unset!')
    if self.currency is None:
      raise TProtocol.TProtocolException(message='Required field currency is unset!')
    return


  def __hash__(self):
    value = 17
    value = (value * 31) ^ hash(self.symbol)
    value = (value * 31) ^ hash(self.secType)
    value = (value * 31) ^ hash(self.exchange)
    value = (value * 31) ^ hash(self.currency)
    return value

  def __repr__(self):
    L = ['%s=%r' % (key, value)
      for key, value in self.__dict__.iteritems()]
    return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

  def __eq__(self, other):
    return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

class OrderRequest:
  """
  Attributes:
   - action
   - totalQuantity
   - orderType
   - lmtPrice
  """

  thrift_spec = (
    None, # 0
    (1, TType.STRING, 'action', None, None, ), # 1
    (2, TType.I64, 'totalQuantity', None, None, ), # 2
    (3, TType.STRING, 'orderType', None, None, ), # 3
    (4, TType.DOUBLE, 'lmtPrice', None, None, ), # 4
  )

  def __init__(self, action=None, totalQuantity=None, orderType=None, lmtPrice=None,):
    self.action = action
    self.totalQuantity = totalQuantity
    self.orderType = orderType
    self.lmtPrice = lmtPrice

  def read(self, iprot):
    if iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastbinary is not None:
      fastbinary.decode_binary(self, iprot.trans, (self.__class__, self.thrift_spec))
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      if fid == 1:
        if ftype == TType.STRING:
          self.action = iprot.readString();
        else:
          iprot.skip(ftype)
      elif fid == 2:
        if ftype == TType.I64:
          self.totalQuantity = iprot.readI64();
        else:
          iprot.skip(ftype)
      elif fid == 3:
        if ftype == TType.STRING:
          self.orderType = iprot.readString();
        else:
          iprot.skip(ftype)
      elif fid == 4:
        if ftype == TType.DOUBLE:
          self.lmtPrice = iprot.readDouble();
        else:
          iprot.skip(ftype)
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and self.thrift_spec is not None and fastbinary is not None:
      oprot.trans.write(fastbinary.encode_binary(self, (self.__class__, self.thrift_spec)))
      return
    oprot.writeStructBegin('OrderRequest')
    if self.action is not None:
      oprot.writeFieldBegin('action', TType.STRING, 1)
      oprot.writeString(self.action)
      oprot.writeFieldEnd()
    if self.totalQuantity is not None:
      oprot.writeFieldBegin('totalQuantity', TType.I64, 2)
      oprot.writeI64(self.totalQuantity)
      oprot.writeFieldEnd()
    if self.orderType is not None:
      oprot.writeFieldBegin('orderType', TType.STRING, 3)
      oprot.writeString(self.orderType)
      oprot.writeFieldEnd()
    if self.lmtPrice is not None:
      oprot.writeFieldBegin('lmtPrice', TType.DOUBLE, 4)
      oprot.writeDouble(self.lmtPrice)
      oprot.writeFieldEnd()
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def validate(self):
    if self.action is None:
      raise TProtocol.TProtocolException(message='Required field action is unset!')
    if self.totalQuantity is None:
      raise TProtocol.TProtocolException(message='Required field totalQuantity is unset!')
    if self.orderType is None:
      raise TProtocol.TProtocolException(message='Required field orderType is unset!')
    if self.lmtPrice is None:
      raise TProtocol.TProtocolException(message='Required field lmtPrice is unset!')
    return


  def __hash__(self):
    value = 17
    value = (value * 31) ^ hash(self.action)
    value = (value * 31) ^ hash(self.totalQuantity)
    value = (value * 31) ^ hash(self.orderType)
    value = (value * 31) ^ hash(self.lmtPrice)
    return value

  def __repr__(self):
    L = ['%s=%r' % (key, value)
      for key, value in self.__dict__.iteritems()]
    return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

  def __eq__(self, other):
    return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

class OrderResponse:
  """
  Attributes:
   - orderId
   - state
   - status
   - filled
   - remaining
   - avgFillPrice
   - permId
   - parentId
   - lastFillPrice
   - clientId
   - whyHeld
  """

  thrift_spec = (
    None, # 0
    (1, TType.I64, 'orderId', None, None, ), # 1
    (2, TType.I16, 'state', None, None, ), # 2
    (3, TType.STRING, 'status', None, "", ), # 3
    (4, TType.I32, 'filled', None, 0, ), # 4
    (5, TType.I32, 'remaining', None, 0, ), # 5
    (6, TType.DOUBLE, 'avgFillPrice', None, 0, ), # 6
    (7, TType.I32, 'permId', None, 0, ), # 7
    (8, TType.I32, 'parentId', None, 0, ), # 8
    (9, TType.DOUBLE, 'lastFillPrice', None, 0, ), # 9
    (10, TType.I32, 'clientId', None, 0, ), # 10
    (11, TType.STRING, 'whyHeld', None, "", ), # 11
  )

  def __init__(self, orderId=None, state=None, status=thrift_spec[3][4], filled=thrift_spec[4][4], remaining=thrift_spec[5][4], avgFillPrice=thrift_spec[6][4], permId=thrift_spec[7][4], parentId=thrift_spec[8][4], lastFillPrice=thrift_spec[9][4], clientId=thrift_spec[10][4], whyHeld=thrift_spec[11][4],):
    self.orderId = orderId
    self.state = state
    self.status = status
    self.filled = filled
    self.remaining = remaining
    self.avgFillPrice = avgFillPrice
    self.permId = permId
    self.parentId = parentId
    self.lastFillPrice = lastFillPrice
    self.clientId = clientId
    self.whyHeld = whyHeld

  def read(self, iprot):
    if iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastbinary is not None:
      fastbinary.decode_binary(self, iprot.trans, (self.__class__, self.thrift_spec))
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      if fid == 1:
        if ftype == TType.I64:
          self.orderId = iprot.readI64();
        else:
          iprot.skip(ftype)
      elif fid == 2:
        if ftype == TType.I16:
          self.state = iprot.readI16();
        else:
          iprot.skip(ftype)
      elif fid == 3:
        if ftype == TType.STRING:
          self.status = iprot.readString();
        else:
          iprot.skip(ftype)
      elif fid == 4:
        if ftype == TType.I32:
          self.filled = iprot.readI32();
        else:
          iprot.skip(ftype)
      elif fid == 5:
        if ftype == TType.I32:
          self.remaining = iprot.readI32();
        else:
          iprot.skip(ftype)
      elif fid == 6:
        if ftype == TType.DOUBLE:
          self.avgFillPrice = iprot.readDouble();
        else:
          iprot.skip(ftype)
      elif fid == 7:
        if ftype == TType.I32:
          self.permId = iprot.readI32();
        else:
          iprot.skip(ftype)
      elif fid == 8:
        if ftype == TType.I32:
          self.parentId = iprot.readI32();
        else:
          iprot.skip(ftype)
      elif fid == 9:
        if ftype == TType.DOUBLE:
          self.lastFillPrice = iprot.readDouble();
        else:
          iprot.skip(ftype)
      elif fid == 10:
        if ftype == TType.I32:
          self.clientId = iprot.readI32();
        else:
          iprot.skip(ftype)
      elif fid == 11:
        if ftype == TType.STRING:
          self.whyHeld = iprot.readString();
        else:
          iprot.skip(ftype)
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and self.thrift_spec is not None and fastbinary is not None:
      oprot.trans.write(fastbinary.encode_binary(self, (self.__class__, self.thrift_spec)))
      return
    oprot.writeStructBegin('OrderResponse')
    if self.orderId is not None:
      oprot.writeFieldBegin('orderId', TType.I64, 1)
      oprot.writeI64(self.orderId)
      oprot.writeFieldEnd()
    if self.state is not None:
      oprot.writeFieldBegin('state', TType.I16, 2)
      oprot.writeI16(self.state)
      oprot.writeFieldEnd()
    if self.status is not None:
      oprot.writeFieldBegin('status', TType.STRING, 3)
      oprot.writeString(self.status)
      oprot.writeFieldEnd()
    if self.filled is not None:
      oprot.writeFieldBegin('filled', TType.I32, 4)
      oprot.writeI32(self.filled)
      oprot.writeFieldEnd()
    if self.remaining is not None:
      oprot.writeFieldBegin('remaining', TType.I32, 5)
      oprot.writeI32(self.remaining)
      oprot.writeFieldEnd()
    if self.avgFillPrice is not None:
      oprot.writeFieldBegin('avgFillPrice', TType.DOUBLE, 6)
      oprot.writeDouble(self.avgFillPrice)
      oprot.writeFieldEnd()
    if self.permId is not None:
      oprot.writeFieldBegin('permId', TType.I32, 7)
      oprot.writeI32(self.permId)
      oprot.writeFieldEnd()
    if self.parentId is not None:
      oprot.writeFieldBegin('parentId', TType.I32, 8)
      oprot.writeI32(self.parentId)
      oprot.writeFieldEnd()
    if self.lastFillPrice is not None:
      oprot.writeFieldBegin('lastFillPrice', TType.DOUBLE, 9)
      oprot.writeDouble(self.lastFillPrice)
      oprot.writeFieldEnd()
    if self.clientId is not None:
      oprot.writeFieldBegin('clientId', TType.I32, 10)
      oprot.writeI32(self.clientId)
      oprot.writeFieldEnd()
    if self.whyHeld is not None:
      oprot.writeFieldBegin('whyHeld', TType.STRING, 11)
      oprot.writeString(self.whyHeld)
      oprot.writeFieldEnd()
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def validate(self):
    if self.orderId is None:
      raise TProtocol.TProtocolException(message='Required field orderId is unset!')
    if self.state is None:
      raise TProtocol.TProtocolException(message='Required field state is unset!')
    return


  def __hash__(self):
    value = 17
    value = (value * 31) ^ hash(self.orderId)
    value = (value * 31) ^ hash(self.state)
    value = (value * 31) ^ hash(self.status)
    value = (value * 31) ^ hash(self.filled)
    value = (value * 31) ^ hash(self.remaining)
    value = (value * 31) ^ hash(self.avgFillPrice)
    value = (value * 31) ^ hash(self.permId)
    value = (value * 31) ^ hash(self.parentId)
    value = (value * 31) ^ hash(self.lastFillPrice)
    value = (value * 31) ^ hash(self.clientId)
    value = (value * 31) ^ hash(self.whyHeld)
    return value

  def __repr__(self):
    L = ['%s=%r' % (key, value)
      for key, value in self.__dict__.iteritems()]
    return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

  def __eq__(self, other):
    return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

class RealTimeBar:
  """
  Attributes:
   - reqId
   - time
   - open
   - low
   - high
   - close
   - volume
   - wap
   - count
  """

  thrift_spec = (
    None, # 0
    (1, TType.I64, 'reqId', None, -2, ), # 1
    (2, TType.I64, 'time', None, 1, ), # 2
    (3, TType.DOUBLE, 'open', None, 1, ), # 3
    (4, TType.DOUBLE, 'low', None, 1, ), # 4
    (5, TType.DOUBLE, 'high', None, 1, ), # 5
    (6, TType.DOUBLE, 'close', None, 1, ), # 6
    (7, TType.I64, 'volume', None, 1, ), # 7
    (8, TType.DOUBLE, 'wap', None, 1, ), # 8
    (9, TType.I32, 'count', None, 1, ), # 9
  )

  def __init__(self, reqId=thrift_spec[1][4], time=thrift_spec[2][4], open=thrift_spec[3][4], low=thrift_spec[4][4], high=thrift_spec[5][4], close=thrift_spec[6][4], volume=thrift_spec[7][4], wap=thrift_spec[8][4], count=thrift_spec[9][4],):
    self.reqId = reqId
    self.time = time
    self.open = open
    self.low = low
    self.high = high
    self.close = close
    self.volume = volume
    self.wap = wap
    self.count = count

  def read(self, iprot):
    if iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastbinary is not None:
      fastbinary.decode_binary(self, iprot.trans, (self.__class__, self.thrift_spec))
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      if fid == 1:
        if ftype == TType.I64:
          self.reqId = iprot.readI64();
        else:
          iprot.skip(ftype)
      elif fid == 2:
        if ftype == TType.I64:
          self.time = iprot.readI64();
        else:
          iprot.skip(ftype)
      elif fid == 3:
        if ftype == TType.DOUBLE:
          self.open = iprot.readDouble();
        else:
          iprot.skip(ftype)
      elif fid == 4:
        if ftype == TType.DOUBLE:
          self.low = iprot.readDouble();
        else:
          iprot.skip(ftype)
      elif fid == 5:
        if ftype == TType.DOUBLE:
          self.high = iprot.readDouble();
        else:
          iprot.skip(ftype)
      elif fid == 6:
        if ftype == TType.DOUBLE:
          self.close = iprot.readDouble();
        else:
          iprot.skip(ftype)
      elif fid == 7:
        if ftype == TType.I64:
          self.volume = iprot.readI64();
        else:
          iprot.skip(ftype)
      elif fid == 8:
        if ftype == TType.DOUBLE:
          self.wap = iprot.readDouble();
        else:
          iprot.skip(ftype)
      elif fid == 9:
        if ftype == TType.I32:
          self.count = iprot.readI32();
        else:
          iprot.skip(ftype)
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and self.thrift_spec is not None and fastbinary is not None:
      oprot.trans.write(fastbinary.encode_binary(self, (self.__class__, self.thrift_spec)))
      return
    oprot.writeStructBegin('RealTimeBar')
    if self.reqId is not None:
      oprot.writeFieldBegin('reqId', TType.I64, 1)
      oprot.writeI64(self.reqId)
      oprot.writeFieldEnd()
    if self.time is not None:
      oprot.writeFieldBegin('time', TType.I64, 2)
      oprot.writeI64(self.time)
      oprot.writeFieldEnd()
    if self.open is not None:
      oprot.writeFieldBegin('open', TType.DOUBLE, 3)
      oprot.writeDouble(self.open)
      oprot.writeFieldEnd()
    if self.low is not None:
      oprot.writeFieldBegin('low', TType.DOUBLE, 4)
      oprot.writeDouble(self.low)
      oprot.writeFieldEnd()
    if self.high is not None:
      oprot.writeFieldBegin('high', TType.DOUBLE, 5)
      oprot.writeDouble(self.high)
      oprot.writeFieldEnd()
    if self.close is not None:
      oprot.writeFieldBegin('close', TType.DOUBLE, 6)
      oprot.writeDouble(self.close)
      oprot.writeFieldEnd()
    if self.volume is not None:
      oprot.writeFieldBegin('volume', TType.I64, 7)
      oprot.writeI64(self.volume)
      oprot.writeFieldEnd()
    if self.wap is not None:
      oprot.writeFieldBegin('wap', TType.DOUBLE, 8)
      oprot.writeDouble(self.wap)
      oprot.writeFieldEnd()
    if self.count is not None:
      oprot.writeFieldBegin('count', TType.I32, 9)
      oprot.writeI32(self.count)
      oprot.writeFieldEnd()
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def validate(self):
    if self.reqId is None:
      raise TProtocol.TProtocolException(message='Required field reqId is unset!')
    if self.time is None:
      raise TProtocol.TProtocolException(message='Required field time is unset!')
    if self.open is None:
      raise TProtocol.TProtocolException(message='Required field open is unset!')
    if self.low is None:
      raise TProtocol.TProtocolException(message='Required field low is unset!')
    if self.high is None:
      raise TProtocol.TProtocolException(message='Required field high is unset!')
    if self.close is None:
      raise TProtocol.TProtocolException(message='Required field close is unset!')
    if self.volume is None:
      raise TProtocol.TProtocolException(message='Required field volume is unset!')
    if self.wap is None:
      raise TProtocol.TProtocolException(message='Required field wap is unset!')
    if self.count is None:
      raise TProtocol.TProtocolException(message='Required field count is unset!')
    return


  def __hash__(self):
    value = 17
    value = (value * 31) ^ hash(self.reqId)
    value = (value * 31) ^ hash(self.time)
    value = (value * 31) ^ hash(self.open)
    value = (value * 31) ^ hash(self.low)
    value = (value * 31) ^ hash(self.high)
    value = (value * 31) ^ hash(self.close)
    value = (value * 31) ^ hash(self.volume)
    value = (value * 31) ^ hash(self.wap)
    value = (value * 31) ^ hash(self.count)
    return value

  def __repr__(self):
    L = ['%s=%r' % (key, value)
      for key, value in self.__dict__.iteritems()]
    return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

  def __eq__(self, other):
    return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)

class Exception(TException):
  """
  Attributes:
   - what
   - why
  """

  thrift_spec = (
    None, # 0
    (1, TType.I32, 'what', None, None, ), # 1
    (2, TType.STRING, 'why', None, None, ), # 2
  )

  def __init__(self, what=None, why=None,):
    self.what = what
    self.why = why

  def read(self, iprot):
    if iprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastbinary is not None:
      fastbinary.decode_binary(self, iprot.trans, (self.__class__, self.thrift_spec))
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      if fid == 1:
        if ftype == TType.I32:
          self.what = iprot.readI32();
        else:
          iprot.skip(ftype)
      elif fid == 2:
        if ftype == TType.STRING:
          self.why = iprot.readString();
        else:
          iprot.skip(ftype)
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if oprot.__class__ == TBinaryProtocol.TBinaryProtocolAccelerated and self.thrift_spec is not None and fastbinary is not None:
      oprot.trans.write(fastbinary.encode_binary(self, (self.__class__, self.thrift_spec)))
      return
    oprot.writeStructBegin('Exception')
    if self.what is not None:
      oprot.writeFieldBegin('what', TType.I32, 1)
      oprot.writeI32(self.what)
      oprot.writeFieldEnd()
    if self.why is not None:
      oprot.writeFieldBegin('why', TType.STRING, 2)
      oprot.writeString(self.why)
      oprot.writeFieldEnd()
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def validate(self):
    return


  def __str__(self):
    return repr(self)

  def __hash__(self):
    value = 17
    value = (value * 31) ^ hash(self.what)
    value = (value * 31) ^ hash(self.why)
    return value

  def __repr__(self):
    L = ['%s=%r' % (key, value)
      for key, value in self.__dict__.iteritems()]
    return '%s(%s)' % (self.__class__.__name__, ', '.join(L))

  def __eq__(self, other):
    return isinstance(other, self.__class__) and self.__dict__ == other.__dict__

  def __ne__(self, other):
    return not (self == other)
