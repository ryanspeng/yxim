#!/bin/env python

import time
import struct
import socket
import hashlib
import stat_pb2

req = stat_pb2.IMServerStatisticReq()
body = req.SerializeToString()
size = len(body)+16

head = struct.pack("!IHHHHHH", size, 1, 0, stat_pb2.SID_OTHER, stat_pb2.CID_OTHER_SERVER_STATISTIC_REQ, 0, 0)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('182.18.40.231', 8000))
s.send(head)
s.send(body)

res = s.recv(8196)

size = len(res)
print size
res_head = res[:16]
res_body = res[16:]

print "Server Statistic:", struct.unpack("!IHHHHHH", res_head)

rsp = stat_pb2.IMServerStatisticRsp()
rsp.ParseFromString(res_body)
print "Server Statistic:"
print rsp
print ""


req = stat_pb2.IMClientListReq()
body = req.SerializeToString()
size = len(body)+16

head = struct.pack("!IHHHHHH", size, 1, 0, stat_pb2.SID_OTHER, stat_pb2.CID_OTHER_CLIENT_LIST_REQ, 0, 0)
s.send(head)
s.send(body)

res = s.recv(8196)

size = len(res)
print size
res_head = res[:16]
res_body = res[16:]

print "Client List:", struct.unpack("!IHHHHHH", res_head)

rsp = stat_pb2.IMClientListRsp()
rsp.ParseFromString(res_body)
print "Client List:"
print rsp
