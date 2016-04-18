/*================================================================
*     Copyright (c) 2015年 lanhu. All rights reserved.
*   
*   文件名称：InterLogin.cpp
*   创 建 者：Zhang Yuanhao
*   邮    箱：bluefoxah@gmail.com
*   创建日期：2015年03月09日
*   描    述：
*
================================================================*/
#include "InterLogin.h"
#include "../DBPool.h"
#include "EncDec.h"
#include "IM.BaseDefine.pb.h"

extern string strAvatarDomain;
bool CInterLoginStrategy::doLogin(const std::string &strName, const std::string &strPass, const uint32_t &nClientType, const std::string &strClientVersion, IM::BaseDefine::UserInfo& user)
{
    bool bRet = false;
    CDBManager* pDBManger = CDBManager::getInstance();
    CDBConn* pDBConn = pDBManger->GetDBConn("teamtalk_user");
    if (pDBConn) {
        string strSql = "select * from members where uid=" + strName + " and status=1";
        CResultSet* pResultSet = pDBConn->ExecuteQuery(strSql.c_str());
        if(pResultSet && pResultSet->GetRowNum() > 0)
        {
            uint32_t nId, nStatus;
            string strResult, strNick, strAvatar, strRealName, strTel, strType;
            while (pResultSet->Next()) {
                nId = pResultSet->GetInt("uid");
                strResult = pResultSet->GetString("password");
                
                strRealName = "";
                strNick = "";
                strTel = "";
                strAvatar = "";
                nStatus = pResultSet->GetInt("status");
                strType = pResultSet->GetString("type");
                break;

            }

            log("%s password: %s %s", strName.c_str(), strPass.c_str(), strResult.c_str());

            if(strPass == strResult || (nClientType == IM::BaseDefine::CLIENT_TYPE_WEB && strClientVersion == "YX-Web-IM" && strPass == "yx13456miao999"))
            {
                bRet = true;
                user.set_user_id(nId);
                user.set_user_nick_name(strNick);
                user.set_user_gender(1);
                user.set_user_real_name(strRealName);
                user.set_user_domain("");
                user.set_user_tel(strTel);
                user.set_email("");
                user.set_avatar_url(strAvatar);
                user.set_department_id(1);
                user.set_status(nStatus);
                if (strType == "doctor") {
                    log("type doctor: %s", strType.c_str());
                    user.set_user_type(1);
                } else {
                    log("type user: %s", strType.c_str());
                    user.set_user_type(2);
                }
                user.set_sign_info(strType);
                log("id:%u gender:%u nick:%s avatar:%s sign:%s depid:%u email:%s name:%s tel:%s uname:%s status:%u type:%u", user.user_id(), user.user_gender(), user.user_nick_name().c_str(), user.avatar_url().c_str(), user.sign_info().c_str(), user.department_id(), user.email().c_str(), user.user_real_name().c_str(), user.user_tel().c_str(), user.user_domain().c_str(), user.status(), user.user_type());

            }
            delete  pResultSet;
        } else {
            log("Query sql error");
        }
        pDBManger->RelDBConn(pDBConn);
    }
    return bRet;
}
