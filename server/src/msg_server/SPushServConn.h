//
//  PushServConn.h
//  im-server-TT
//
//  Created by luoning on 14-9-15.
//  Copyright (c) 2014年 luoning. All rights reserved.
//

#ifndef SPushServConn_H
#define SPushServConn_H

#include <iostream>

#include "imconn.h"
#include "ServInfo.h"

class CSPushServConn : public CImConn
{
public:
	CSPushServConn();
	virtual ~CSPushServConn();
    
	bool IsOpen() { return m_bOpen; }
    
	void Connect(const char* server_ip, uint16_t server_port, uint32_t serv_idx);
	virtual void Close();
    
	virtual void OnConfirm();
	virtual void OnClose();
	virtual void OnTimer(uint64_t curr_tick);
    
	virtual void HandlePdu(CImPdu* pPdu);
private:
	void _HandlePushToUserResponse(CImPdu* pPdu);

private:
	bool 		m_bOpen;
	uint32_t	m_serv_idx;
};

void init_s_push_serv_conn(serv_info_t* server_list, uint32_t server_count);
CSPushServConn* get_s_push_serv_conn();

void build_ios_s_push_flash(string& flash, uint32_t msg_type, uint32_t from_id);
#endif /* defined(__im_server_TT__SPushServConn__) */
