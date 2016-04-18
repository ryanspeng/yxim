/*================================================================
 *   Copyright (C) 2014 All rights reserved.
 *
 *   文件名称：MessageCounter.cpp
 *   创 建 者：Zhang Yuanhao
 *   邮    箱：bluefoxah@gmail.com
 *   创建日期：2014年12月15日
 *   描    述：
 *
 ================================================================*/

#include "../ProxyConn.h"
#include "../CachePool.h"
#include "MessageCounter.h"
#include "MessageModel.h"
#include "GroupMessageModel.h"
#include "IM.Message.pb.h"
#include "IM.BaseDefine.pb.h"
#include "IM.Login.pb.h"
#include "IM.Server.pb.h"
#include "UserModel.h"
#include<time.h>

namespace DB_PROXY {

    void getUnreadMsgCounter(CImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Message::IMUnreadMsgCntReq msg;
        IM::Message::IMUnreadMsgCntRsp msgResp;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            CImPdu* pPduResp = new CImPdu;

            uint32_t nUserId = msg.user_id();

            list<IM::BaseDefine::UnreadInfo> lsUnreadCount;
            uint32_t nTotalCnt = 0;
            
            CMessageModel::getInstance()->getUnreadMsgCount(nUserId, nTotalCnt, lsUnreadCount);
            CGroupMessageModel::getInstance()->getUnreadMsgCount(nUserId, nTotalCnt, lsUnreadCount);
            msgResp.set_user_id(nUserId);
            msgResp.set_total_cnt(nTotalCnt);
            for(auto it= lsUnreadCount.begin(); it!=lsUnreadCount.end(); ++it)
            {
                IM::BaseDefine::UnreadInfo* pInfo = msgResp.add_unreadinfo_list();
    //            *pInfo = *it;
                pInfo->set_session_id(it->session_id());
                pInfo->set_session_type(it->session_type());
                pInfo->set_unread_cnt(it->unread_cnt());
                pInfo->set_latest_msg_id(it->latest_msg_id());
                pInfo->set_latest_msg_data(it->latest_msg_data());
                pInfo->set_latest_msg_type(it->latest_msg_type());
                pInfo->set_latest_msg_from_user_id(it->latest_msg_from_user_id());
            }
            
            
            log("userId=%d, unreadCnt=%u, totalCount=%u", nUserId, msgResp.unreadinfo_list_size(), nTotalCnt);
            msgResp.set_attach_data(msg.attach_data());
            pPduResp->SetPBMsg(&msgResp);
            pPduResp->SetSeqNum(pPdu->GetSeqNum());
            pPduResp->SetServiceId(IM::BaseDefine::SID_MSG);
            pPduResp->SetCommandId(IM::BaseDefine::CID_MSG_UNREAD_CNT_RESPONSE);
            CProxyConn::AddResponsePdu(conn_uuid, pPduResp);
        }
        else
        {
            log("parse pb failed");
        }
    }

    void clearUnreadMsgCounter(CImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Message::IMMsgDataReadAck msg;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            uint32_t nUserId = msg.user_id();
            uint32_t nFromId = msg.session_id();
            IM::BaseDefine::SessionType nSessionType = msg.session_type();
            CUserModel::getInstance()->clearUserCounter(nUserId, nFromId, nSessionType);
            log("userId=%u, peerId=%u, type=%u", nFromId, nUserId, nSessionType);
        }
        else
        {
            log("parse pb failed");
        }
    }

    bool setInternalDevicesToken(uint32_t nUserId, string strToken, IM::BaseDefine::ClientType nClientType)
    {
        bool bRet = false;
        CacheManager* pCacheManager = CacheManager::getInstance();
        CacheConn* pCacheConn = pCacheManager->GetCacheConn("token");
        if (pCacheConn)
        {
            string strValue;
            if(IM::BaseDefine::CLIENT_TYPE_IOS == nClientType)
            {
                strValue = "ios:"+strToken;
            }
            else if(IM::BaseDefine::CLIENT_TYPE_ANDROID == nClientType)
            {
                strValue = "android:"+strToken;
            }
            else
            {
                strValue = strToken;
            }
            
            string strOldValue = pCacheConn->get("device_"+int2string(nUserId));
            
            if(!strOldValue.empty())
            {
                size_t nPos = strOldValue.find(":");
                if(nPos!=string::npos)
                {
                    string strOldToken = strOldValue.substr(nPos + 1);
                    string strReply = pCacheConn->get("device_"+strOldToken);
                    if (!strReply.empty()) {
                        string strNewValue("");
                        pCacheConn->set("device_"+strOldToken, strNewValue);
                    }
                }
            }
            
            pCacheConn->set("device_"+int2string(nUserId), strValue);
            string strNewValue = int2string(nUserId);
            pCacheConn->set("device_"+strToken, strNewValue);
        
            pCacheManager->RelCacheConn(pCacheConn);
            log("setDeviceToken. userId=%u, deviceToken=%s", nUserId, strToken.c_str());
            bRet = true;
        }
        else
        {
            log("no cache connection for token");
        }
        return bRet;
    }

    void setDevicesToken(CImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Login::IMDeviceTokenReq msg;
        IM::Login::IMDeviceTokenRsp msgResp;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            uint32_t nUserId = msg.user_id();
            string strToken = msg.device_token();
            IM::BaseDefine::ClientType nClientType = msg.client_type();
            bool bRet = setInternalDevicesToken(nUserId, strToken, nClientType);
            CImPdu* pPduResp = new CImPdu;
            
            log("setDeviceToken. userId=%u, deviceToken=%s", nUserId, strToken.c_str());
            msgResp.set_attach_data(msg.attach_data());
            msgResp.set_user_id(nUserId);
            pPduResp->SetPBMsg(&msgResp);
            pPduResp->SetSeqNum(pPdu->GetSeqNum());
            pPduResp->SetServiceId(IM::BaseDefine::SID_LOGIN);
            pPduResp->SetCommandId(IM::BaseDefine::CID_LOGIN_RES_DEVICETOKEN);
            CProxyConn::AddResponsePdu(conn_uuid, pPduResp);
        }
        else
        {
            log("parse pb failed");
        }
    }


    void getDevicesToken(CImPdu* pPdu, uint32_t conn_uuid)
    {
        IM::Server::IMGetDeviceTokenReq msg;
        IM::Server::IMGetDeviceTokenRsp msgResp;
        if(msg.ParseFromArray(pPdu->GetBodyData(), pPdu->GetBodyLength()))
        {
            CacheManager* pCacheManager = CacheManager::getInstance();
            CacheConn* pCacheConn = pCacheManager->GetCacheConn("token");
            CImPdu* pPduResp = new CImPdu;
            uint32_t nCnt = msg.user_id_size();
            
            // 对于ios，不推送
            // 对于android，由客户端处理
            if (pCacheConn)
            {
                vector<string> vecTokens;
                for (uint32_t i=0; i<nCnt; ++i) {
                    string strKey = "device_"+int2string(msg.user_id(i));
                    vecTokens.push_back(strKey);
                }
                map<string, string> mapTokens;
                bool bRet = pCacheConn->mget(vecTokens, mapTokens);
                pCacheManager->RelCacheConn(pCacheConn);
                
                if(bRet)
                {
                    for (auto it=mapTokens.begin(); it!=mapTokens.end(); ++it) {
                        string strKey = it->first;
                        size_t nPos = strKey.find("device_");
                        if( nPos != string::npos)
                        {
                            string strUserId = strKey.substr(nPos + strlen("device_"));
                            uint32_t nUserId = string2int(strUserId);
                            uint32_t nType = 0;
                            string strValue = it->second;
                            nPos = strValue.find(":");
                            if(nPos!=string::npos)
                            {
                                string strType = strValue.substr(0, nPos);
                                string strToken = strValue.substr(nPos + 1);
                                IM::BaseDefine::ClientType nClientType = IM::BaseDefine::ClientType(0);
                                CUserModel::getInstance()->getUserType(nUserId, &nType);
                                if(strType == "ios")
                                {
                                    // 检查勿扰状态，状态为1，则屏蔽Push提醒
                                    uint32_t shield_status = 0;
                                    CUserModel::getInstance()->getPushShield(nUserId, &shield_status);
                                    
                                    if (shield_status == 1) {
                                        // 对IOS处理
                                        continue;
                                    } else {
                                        nClientType = IM::BaseDefine::CLIENT_TYPE_IOS;
                                    }
                                    
                                    // nClientType = IM::BaseDefine::CLIENT_TYPE_IOS;
                                    // end
                                }
                                else if(strType == "android")
                                {
                                    nClientType = IM::BaseDefine::CLIENT_TYPE_ANDROID;
                                }
                                if(IM::BaseDefine::ClientType_IsValid(nClientType))
                                {
                                    IM::BaseDefine::UserTokenInfo* pToken = msgResp.add_user_token_info();
                                    pToken->set_user_id(nUserId);
                                    pToken->set_token(strToken);
                                    pToken->set_user_type(nClientType);
                                    pToken->set_user_role(nType);
                                    uint32_t nTotalCnt = 0;
                                    CMessageModel::getInstance()->getUnReadCntAll(nUserId, nTotalCnt);
                                    CGroupMessageModel::getInstance()->getUnReadCntAll(nUserId, nTotalCnt);
                                    pToken->set_push_count(nTotalCnt);
                                    pToken->set_push_type(1);
                                    log("send UserTokenInfo: id=%u token=%s client_type=%u role=%u", nUserId, strToken.c_str(), nClientType, nType);
                                }
                                else
                                {
                                    log("invalid clientType.clientType=%u", nClientType);
                                }
                            }
                            else
                            {
                                log("invalid value. value=%s", strValue.c_str());
                            }
                            
                        }
                        else
                        {
                            log("invalid key.key=%s", strKey.c_str());
                        }
                    }
                }
                else
                {
                    log("mget failed!");
                }
            }
            else
            {
                log("no cache connection for token");
            }
            
            log("req devices token.reqCnt=%u, resCnt=%u", nCnt, msgResp.user_token_info_size());
            
            msgResp.set_attach_data(msg.attach_data());
            pPduResp->SetPBMsg(&msgResp);
            pPduResp->SetSeqNum(pPdu->GetSeqNum());
            pPduResp->SetServiceId(IM::BaseDefine::SID_OTHER);
            pPduResp->SetCommandId(IM::BaseDefine::CID_OTHER_GET_DEVICE_TOKEN_RSP);
            CProxyConn::AddResponsePdu(conn_uuid, pPduResp);
        }
        else
        {
            log("parse pb failed");
        }
    }
};


