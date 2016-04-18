#!/bin/env python
# -*- encoding: utf-8 -*-

import sys
import json
import time
import socket
import struct
import hashlib
import traceback

import tornado.ioloop
import tornado.web
import IM_pb2

reload(sys)
sys.setdefaultencoding('utf-8')

def send_to_msgserver(sock, service_id, command_id, request):
    body = request.SerializeToString()

    size = len(body) + 16
    head = struct.pack("!IHHHHHH", size, 1, 0, service_id, command_id, 0, 0)

    sock.send(head)
    sock.send(body)

    #rcv_data = sock.recv(8196)
    #rcv_head = struct.unpack("!IHHHHHH", rcv_data[:16])
    size_data = sock.recv(4)
    size = struct.unpack("!I", size_data)[0]
    data = sock.recv(size-4)
    head = [size]
    head.extend(list(struct.unpack("!HHHHHH", data[:12])))
    return head, data[12:]

class SendMsgHandler(tornado.web.RequestHandler):
    def post(self):
        try:
            msg_info = json.loads(self.request.body)
            if any([x not in msg_info for x in ('from_id', 'to_id', 'msg_data')]):
                rsp_info = {'error_code': 2, 'error_msg': '请求格式有误'}
                return
            print "json data:", msg_info

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('127.0.0.1', 8000))

            #user login
            req = IM_pb2.IMLoginReq()
            req.user_name = str(msg_info['from_id'])
            req.password = ""
            req.online_status = IM_pb2.USER_STATUS_ONLINE
            req.client_type = IM_pb2.CLIENT_TYPE_WEB
            req.client_version = ""
            header, body = send_to_msgserver(sock, IM_pb2.SID_LOGIN, IM_pb2.CID_LOGIN_REQ_USERLOGIN, req)
            print "login header:", header

            rsp = IM_pb2.IMLoginRes()
            rsp.ParseFromString(body)
            if rsp.result_code != 0:
                print "code: %s errmsg: %s" % (rsp.result_code, rsp.result_string)
                rsp_info = {'error_code': 3, 'error_msg': '登录失败'}
                sock.close()
                return

            #send message
            req = IM_pb2.IMMsgData()
            req.from_user_id = int(msg_info['from_id'])
            req.to_session_id = int(msg_info['to_id'])
            req.msg_id = int(msg_info.get('msg_id', 0))
            req.create_time = int(msg_info.get('create_time', time.time()))
            req.msg_type = IM_pb2.MSG_TYPE_SINGLE_TEXT
            req.msg_data = msg_info['msg_data'].encode('utf8')
            if 'msg_id' in msg_info:
                req.forwarded = 1
            header, body = send_to_msgserver(sock, IM_pb2.SID_MSG, IM_pb2.CID_MSG_DATA, req)
            print "send message header:", header
            if header[4] == IM_pb2.CID_MSG_DATA_ACK:
                rsp = IM_pb2.IMMsgDataAck()
                rsp.ParseFromString(body)
                rsp_info = {'error_code': 0, 'error_msg': '', 'msg_id': rsp.msg_id}
                return

            rsp_info = {'error_code': 0, 'error_msg': ''}
            sock.close()
        except:
            print traceback.format_exc()
            rsp_info = {'error_code': 1, 'error_msg': '发消息异常'}
        finally:
            self.write(json.dumps(rsp_info))

class GetMsgHandler(tornado.web.RequestHandler):
    def post(self):
        try:
            msg_info = json.loads(self.request.body)
            if any([x not in msg_info for x in ('from_id', 'to_id', 'start_time', 'end_time', 'size')]):
                rsp_info = {'error_code': 2, 'error_msg': '请求格式有误'}
                return
            print "json data:", msg_info

            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(('127.0.0.1', 8000))
            sock.settimeout(2)

            #user login
            req = IM_pb2.IMLoginReq()
            req.user_name = str(msg_info['from_id'])
            req.password = ""
            req.online_status = IM_pb2.USER_STATUS_ONLINE
            req.client_type = IM_pb2.CLIENT_TYPE_WEB
            req.client_version = ""
            header, body = send_to_msgserver(sock, IM_pb2.SID_LOGIN, IM_pb2.CID_LOGIN_REQ_USERLOGIN, req)
            print "login header:", header

            rsp = IM_pb2.IMLoginRes()
            rsp.ParseFromString(body)
            if rsp.result_code != 0:
                print "code: %s errmsg: %s" % (rsp.result_code, rsp.result_string)
                rsp_info = {'error_code': 3, 'error_msg': '登录失败'}
                sock.close()
                return

            #get message
            req = IM_pb2.IMGetTimeMsgListReq()
            req.user_id = int(msg_info['from_id'])
            req.session_id = int(msg_info['to_id'])
            req.session_type = IM_pb2.SESSION_TYPE_SINGLE
            req.start_time = int(msg_info['start_time'])
            req.end_time = int(msg_info['end_time'])
            req.offset = int(msg_info.get('offset', 0))
            req.count = int(msg_info['size'])
            header, body = send_to_msgserver(sock, IM_pb2.SID_MSG, IM_pb2.CID_MSG_GET_DURING_TIME_REQ, req)
            print "get message header:", header

            if header[4] == IM_pb2.CID_MSG_GET_DURING_TIME_RES:
                relation = {req.user_id: req.session_id, req.session_id: req.user_id}
                rsp = IM_pb2.IMGetTimeMsgListRsp()
                rsp.ParseFromString(body)
                l = []
                for msg in rsp.msg_list:
                    l.append({'from_id': msg.from_session_id, 'to_id': relation[msg.from_session_id], 'msg_id': msg.msg_id, 'create_time': msg.create_time, 'msg_data': msg.msg_data})

            rsp_info = {'error_code': 0, 'msg_info': l}
            sock.close()
        except:
            print traceback.format_exc()
            rsp_info = {'error_code': 1, 'error_msg': '获取消息异常'}
        finally:
            self.write(json.dumps(rsp_info))

if __name__ == "__main__":
    app = tornado.web.Application([(r"/IM/SendMessage", SendMsgHandler), (r"/IM/GetMessage", GetMsgHandler)])
    app.listen(8400, address="127.0.0.1")
    tornado.ioloop.IOLoop.current().start()
