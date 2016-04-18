extern "C" {
#include "ImPduBase.h"
#include "public_define.h"
#include "IM.Login.pb.h"
#include "IM.Message.pb.h"

CImPdu login(const string &strName, const string &strPass)
{
    CImPdu cPdu;
    IM::Login::IMLoginReq msg;
    msg.set_user_name(strName);
    msg.set_password(strPass);
    msg.set_online_status(IM::BaseDefine::USER_STATUS_ONLINE);
    msg.set_client_type(IM::BaseDefine::CLIENT_TYPE_WINDOWS);
    msg.set_client_version("1.0");
    cPdu.SetPBMsg(&msg);
    cPdu.SetServiceId(IM::BaseDefine::SID_LOGIN);
    cPdu.SetCommandId(IM::BaseDefine::CID_LOGIN_REQ_USERLOGIN);
    cPdu.SetSeqNum(1);
    return cPdu;
}

CImPdu sendMessage(uint32_t nFromId, uint32_t nToId, const string& strMsgData)
{
    CImPdu cPdu;
    IM::Message::IMMsgData msg;
    msg.set_from_user_id(nFromId);
    msg.set_to_session_id(nToId);
    msg.set_msg_id(0);
    msg.set_create_time(time(NULL));
    msg.set_msg_type(IM::BaseDefine::MSG_TYPE_SINGLE_TEXT);
    msg.set_msg_data(strMsgData);
    cPdu.SetPBMsg(&msg);
    cPdu.SetServiceId(IM::BaseDefine::SID_MSG);
    cPdu.SetCommandId(IM::BaseDefine::CID_MSG_DATA);
    cPdu.SetSeqNum(2);
    return cPdu;
}
}

int main(int argc, char* argv[])
{
    login("3000053", "123456");
    sendMessage(3000053, 3172575, "Hello");
}
