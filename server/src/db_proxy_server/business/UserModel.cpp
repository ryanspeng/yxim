/*================================================================
*     Copyright (c) 2015年 lanhu. All rights reserved.
*   
*   文件名称：UserModel.cpp
*   创 建 者：Zhang Yuanhao
*   邮    箱：bluefoxah@gmail.com
*   创建日期：2015年01月05日
*   描    述：
*
================================================================*/
#include "UserModel.h"
#include "../DBPool.h"
#include "../CachePool.h"
#include "Common.h"
#include "SyncCenter.h"

extern string strAvatarDomain;

CUserModel* CUserModel::m_pInstance = NULL;

CUserModel::CUserModel()
{

}

CUserModel::~CUserModel()
{
    
}

CUserModel* CUserModel::getInstance()
{
    if(m_pInstance == NULL)
    {
        m_pInstance = new CUserModel();
    }
    return m_pInstance;
}

void CUserModel::getRelationId(uint32_t nUserId, uint32_t& nLastTime, list<uint32_t> &lsIds)
{
    CDBManager* pDBManager = CDBManager::getInstance();
    CDBConn* pDBConn = pDBManager->GetDBConn("teamtalk_slave");
    if (pDBConn)
    {
        string strSql;
        if (nLastTime == 0)
        {
            strSql = "select smallId, bigId from IMRelationShip where status = 0 and (smallId = " + int2string(nUserId) + " or bigId = " + int2string(nUserId) +")";
        }
        else
        {
            strSql = "select smallId, bigId from IMRelationShip where status = 0 and (smallId = " + int2string(nUserId) + " or bigId = " + int2string(nUserId) +") and updated >= " + int2string(nLastTime);
        }
        CResultSet* pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->Next()) {
                uint32_t nSmallId = pResultSet->GetInt("smallId");
                uint32_t nBigId = pResultSet->GetInt("bigId");
                if (nSmallId == nUserId)
                {
                    lsIds.push_back(nBigId);
                }
                else
                {
                    lsIds.push_back(nSmallId);
                }
            }
            delete pResultSet;
        }
        else
        {
            log(" no result set for sql:%s", strSql.c_str());
        }
        pDBManager->RelDBConn(pDBConn);
    }
    else
    {
        log("no db connection for teamtalk_slave");
    }
}

void CUserModel::getUsers(list<uint32_t> lsIds, list<IM::BaseDefine::UserInfo> &lsUsers)
{
    if (lsIds.empty()) {
        log("list is empty");
        return;
    }
    CDBManager* pDBManager = CDBManager::getInstance();
    CDBConn* pDBConn = pDBManager->GetDBConn("teamtalk_user");
    if (pDBConn)
    {
        string strClause;
        bool bFirst = true;
        for (auto it = lsIds.begin(); it!=lsIds.end(); ++it)
        {
            if(bFirst)
            {
                bFirst = false;
                strClause += int2string(*it);
            }
            else
            {
                strClause += ("," + int2string(*it));
            }
        }
        string  strSql = "select * from members where uid in (" + strClause + ")";
        CResultSet* pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->Next())
            {
                IM::BaseDefine::UserInfo cUser;
                cUser.set_user_id(pResultSet->GetInt("uid"));
                cUser.set_user_real_name("");
                cUser.set_user_nick_name("");
                cUser.set_user_tel("");
                cUser.set_avatar_url("");
                cUser.set_status(pResultSet->GetInt("status"));

                string strType = pResultSet->GetString("type");
                if (strType == "doctor") {
                    cUser.set_user_type(1);
                } else {
                    cUser.set_user_type(2);
                }
                lsUsers.push_back(cUser);
            }
            delete pResultSet;
        }
        else
        {
            log(" no result set for sql:%s", strSql.c_str());
        }
        pDBManager->RelDBConn(pDBConn);
    }
    else
    {
        log("no db connection for teamtalk_user");
    }
}

bool CUserModel::getUser(uint32_t nUserId, DBUserInfo_t &cUser)
{
    bool bRet = false;
    CDBManager* pDBManager = CDBManager::getInstance();
    CDBConn* pDBConn = pDBManager->GetDBConn("teamtalk_user");
    if (pDBConn)
    {
        string strSql = "select * from members where uid="+int2string(nUserId);
        CResultSet* pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
        if(pResultSet)
        {
            while (pResultSet->Next())
            {
                cUser.nId = pResultSet->GetInt("uid");
                cUser.strName = "";
                cUser.strNick = "";
                cUser.strTel = "";
                cUser.strAvatar = "";
                cUser.nStatus = pResultSet->GetInt("status");

                string strType = pResultSet->GetString("type");
                if (strType == "doctor") {
                    cUser.nType = 1;
                } else {
                    cUser.nType = 2;
                }
                bRet = true;
            }
            delete pResultSet;
        }
        else
        {
            log("no result set for sql:%s", strSql.c_str());
        }
        pDBManager->RelDBConn(pDBConn);
    }
    else
    {
        log("no db connection for teamtalk_user");
    }
    return bRet;
}


bool CUserModel::updateUser(DBUserInfo_t &cUser)
{
    bool bRet = false;
    log("No support updateUser");
    return bRet;
}

bool CUserModel::insertUser(DBUserInfo_t &cUser)
{
    bool bRet = false;
    log("No support insertUser");
    return bRet;
}

void CUserModel::clearUserCounter(uint32_t nUserId, uint32_t nPeerId, IM::BaseDefine::SessionType nSessionType)
{
    if(IM::BaseDefine::SessionType_IsValid(nSessionType))
    {
        CacheManager* pCacheManager = CacheManager::getInstance();
        CacheConn* pCacheConn = pCacheManager->GetCacheConn("unread");
        if (pCacheConn)
        {
            // Clear P2P msg Counter
            if(nSessionType == IM::BaseDefine::SESSION_TYPE_SINGLE)
            {
                int nRet = pCacheConn->hdel("unread_" + int2string(nUserId), int2string(nPeerId));
                if(!nRet)
                {
                    log("hdel failed %d->%d", nPeerId, nUserId);
                }
            }
            // Clear Group msg Counter
            else if(nSessionType == IM::BaseDefine::SESSION_TYPE_GROUP)
            {
                string strGroupKey = int2string(nPeerId) + GROUP_TOTAL_MSG_COUNTER_REDIS_KEY_SUFFIX;
                map<string, string> mapGroupCount;
                bool bRet = pCacheConn->hgetAll(strGroupKey, mapGroupCount);
                if(bRet)
                {
                    string strUserKey = int2string(nUserId) + "_" + int2string(nPeerId) + GROUP_USER_MSG_COUNTER_REDIS_KEY_SUFFIX;
                    string strReply = pCacheConn->hmset(strUserKey, mapGroupCount);
                    if(strReply.empty()) {
                        log("hmset %s failed !", strUserKey.c_str());
                    }
                }
                else
                {
                    log("hgetall %s failed!", strGroupKey.c_str());
                }
                
            }
            pCacheManager->RelCacheConn(pCacheConn);
        }
        else
        {
            log("no cache connection for unread");
        }
    }
    else{
        log("invalid sessionType. userId=%u, fromId=%u, sessionType=%u", nUserId, nPeerId, nSessionType);
    }
}

void CUserModel::setCallReport(uint32_t nUserId, uint32_t nPeerId, IM::BaseDefine::ClientType nClientType)
{
    if(IM::BaseDefine::ClientType_IsValid(nClientType))
    {
        CDBManager* pDBManager = CDBManager::getInstance();
        CDBConn* pDBConn = pDBManager->GetDBConn("teamtalk_master");
        if(pDBConn)
        {
            string strSql = "insert into IMCallLog(`userId`, `peerId`, `clientType`,`created`,`updated`) values(?,?,?,?,?)";
            CPrepareStatement* stmt = new CPrepareStatement();
            if (stmt->Init(pDBConn->GetMysql(), strSql))
            {
                uint32_t nNow = (uint32_t) time(NULL);
                uint32_t index = 0;
                uint32_t nClient = (uint32_t) nClientType;
                stmt->SetParam(index++, nUserId);
                stmt->SetParam(index++, nPeerId);
                stmt->SetParam(index++, nClient);
                stmt->SetParam(index++, nNow);
                stmt->SetParam(index++, nNow);
                bool bRet = stmt->ExecuteUpdate();
                
                if (!bRet)
                {
                    log("insert report failed: %s", strSql.c_str());
                }
            }
            delete stmt;
            pDBManager->RelDBConn(pDBConn);
        }
        else
        {
            log("no db connection for teamtalk_master");
        }
        
    }
    else
    {
        log("invalid clienttype. userId=%u, peerId=%u, clientType=%u", nUserId, nPeerId, nClientType);
    }
}

bool CUserModel::getUserType(uint32_t user_id, uint32_t* user_type) {
    bool rv = false;

    CDBManager* db_manager = CDBManager::getInstance();
    CDBConn* db_conn = db_manager->GetDBConn("teamtalk_user");
    if (db_conn) {
        string str_sql = "select type from members where uid="+int2string(user_id);
        CResultSet* result_set = db_conn->ExecuteQuery(str_sql.c_str());
        if(result_set) {
            if (result_set->Next()) {
                string nType = result_set->GetString("type");
                if (nType == "doctor") {
                    *user_type = 1;
                } else {
                    *user_type = 2;
                }
                rv = true;
            }
            delete result_set;
        } else {
            log("getPushShield: no result set for sql:%s", str_sql.c_str());
        }
        db_manager->RelDBConn(db_conn);
    } else {
        log("getPushShield: no db connection for teamtalk_slave");
    }

    return rv;
}

bool CUserModel::updateUserSignInfo(uint32_t user_id, const string& sign_info) {
    bool rv = false;
    log("No support updateUserSignInfo");
    return rv;
}

bool CUserModel::getUserSignInfo(uint32_t user_id, string* sign_info) {
    bool rv = false;
    log("No support getUserSignInfo");
    return rv;
}

bool CUserModel::updatePushShield(uint32_t user_id, uint32_t shield_status) {
    bool rv = false;
    log("No support updatePushShield");
    return rv;
}

bool CUserModel::getPushShield(uint32_t user_id, uint32_t* shield_status) {
    bool rv = false;

    CDBManager* db_manager = CDBManager::getInstance();
    CDBConn* db_conn = db_manager->GetDBConn("teamtalk_slave");
    if (db_conn) {
        string str_sql = "select push_shield_status from IMUser where id="+int2string(user_id);
        CResultSet* result_set = db_conn->ExecuteQuery(str_sql.c_str());
        if(result_set) {
            if (result_set->Next()) {
                *shield_status = result_set->GetInt("push_shield_status");
                rv = true;
            }
            delete result_set;
        } else {
            log("getPushShield: no result set for sql:%s", str_sql.c_str());
        }
        db_manager->RelDBConn(db_conn);
    } else {
        log("getPushShield: no db connection for teamtalk_slave");
    }

    return rv;
}

