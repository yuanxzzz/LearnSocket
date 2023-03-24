#pragma warning(disable:4996)

#include "ChatRoomClient.h"

ChatRoomClient::ChatRoomClient()
{
	m_lastPos = 0;
	m_isRunnning = true;
}

ChatRoomClient::~ChatRoomClient()
{
	cout << "关闭客户端" << endl;
	shutdown(m_socket, 2);
	Clear();
}

/**
 * \brief 清理资源
 */
void ChatRoomClient::Clear()
{
	if (m_socket)
	{
		closesocket(m_socket);
	}
	if (m_hThread)
	{
		CloseHandle(m_hThread);
	}
	WSACleanup();
}

/**
 * \brief 运行
 * \return 
 */
int ChatRoomClient::Run()
{
	// 初始化socket
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// 设置ip和端口
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PORT);
	addrServer.sin_addr.s_addr = inet_addr(IP);

	if (connect(m_socket, (sockaddr*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR)
	{
		cout << "连接服务器失败..." << endl;
		closesocket(m_socket);
		return -1;
	}
	else
	{
		cout << "连接服务器成功..." << endl;
	}

	// 开启线程进行接收消息
	m_hThread = CreateThread(NULL, NULL, ThreadProc, this, NULL, NULL);

	// 主线程进行发送
	PostSend();
}

/**
 * \brief 接收
 * \return 
 */
int ChatRoomClient::PostRecv()
{
	while (m_isRunnning)
	{
		int recvLen = recv(m_socket, m_buffRecv + m_lastPos, MAX_BUFF_LEN - m_lastPos, 0);
		if (m_isRunnning && recvLen > 0)
		{
			MsgBase* phead = (MsgBase*)m_buffRecv;
			m_lastPos = m_lastPos + recvLen;
			// 循环拆包
			while (m_lastPos >= sizeof(MsgBase))
			{
				if (m_lastPos >= phead->m_dataLen)
				{
					DealWithMsg(phead);
					memcpy(m_buffRecv, m_buffRecv + phead->m_dataLen, m_lastPos - phead->m_dataLen);
					m_lastPos -= phead->m_dataLen;
				}
				else
				{
					break;
				}
			}
		}
		else
		{
			// 接收出错
			cout << "接收消息出错或断开连接!" << endl;
			m_isRunnning = false;
			return -1;
		}
	}
	return 0;
}

/**
 * \brief 发送
 * \return 
 */
int ChatRoomClient::PostSend()
{
	cout << "显示房间列表\t加入房间\t创建房间\t进行聊天\t退出聊天室\t退出客户端" << endl;
	cout << "show\t\tjoin\t\tcreate\t\ttalk\t\tleave\t\texit" << endl;

	while (m_isRunnning)
	{
		m_buffSend[MAX_BUFF_LEN] = { 0 };
		cin >> m_buffSend;
		// 显示聊天室
		if (strcmp(m_buffSend, "show") == 0)
		{
			MsgShow msgShow;
			SendMsg(&msgShow);
		}
		// 加入聊天室
		else if (strncmp(m_buffSend, "join", strlen("join")) == 0)
		{
			// 截取房间号
			int r = atoi(m_buffSend + strlen("join"));
			MsgJoin msgJoin(r);
			SendMsg(&msgJoin);
		}
		// 创建聊天室
		else if (strcmp(m_buffSend, "create") == 0)
		{
			MsgCreate msgCreate;
			SendMsg(&msgCreate);
		}
		// 聊天
		else if (strcmp(m_buffSend, "talk") == 0)
		{
			cout << "开始聊天（输入 leave 退出聊天)" << endl;
			while (true)
			{
				MsgTalk msgTalk;
				msgTalk.SetId(m_id);
				//gets_s(msgTalk.GetBuff(), 1000);
				cin >> msgTalk.GetBuff();
				if (strcmp(msgTalk.GetBuff(), "leave") == 0) {
					cout << "结束聊天，退出聊天室" << endl;
					MsgLeave msgLeave;
					SendMsg(&msgLeave);
					break;
				}
				// 不为空才发送
				if(msgTalk.GetBuff()[0] != '\0')
				{
					SendMsg(&msgTalk);
				}
			}
		}
		// 退出聊天室
		else if (strcmp(m_buffSend, "leave") == 0)
		{
			cout << "结束聊天，退出聊天室" << endl;
			MsgLeave msgLeave;
			SendMsg(&msgLeave);
		}
		// 退出程序
		else if (strcmp(m_buffSend, "exit") == 0)
		{
			m_isRunnning = false;
		}
	}
	return 0;
}

int ChatRoomClient::SendMsg(MsgBase* msgBase)
{
	return(send(m_socket, (const char*)msgBase, msgBase->m_dataLen, 0));
}

/**
 * \brief 处理消息
 * \param msgBase 消息指针 
 */
void ChatRoomClient::DealWithMsg(MsgBase* msgBase)
{
	switch (msgBase->m_msgType)
	{
	case MSG_SHWO:
		cout << "显示请求聊天室" << endl;
		break;
	case MSG_JOIN:
	{
		MsgJoin* join = (MsgJoin*)msgBase;
		cout << "请求加入 " << join->m_roomID << " 聊天室" << endl;
	}
	break;
	case MSG_CREATE:
		cout << "请求创建聊天室" << endl;
		break;
	case MSG_TALK:
	{
		MsgTalk* msgTalk = (MsgTalk*)msgBase;
		if (msgTalk->GetId() != m_id)
		{
			cout << "用户ID[" << msgTalk->GetId() << "]：" << msgTalk->GetBuff() << endl;
		}
	}
	break;
	case MSG_BROAD:
	{
		MsgBroad* broad = (MsgBroad*)msgBase;
		cout << broad->getBuff() << endl;
	}
	break;
	case MSG_LEAVE:
	{
		cout << "离开聊天室..." << endl;
		break;
	}
	case MSG_SHOWREPLY:
	{
		MsgShowReply* showReply = (MsgShowReply*)msgBase;
		cout << "=============展示聊天室信息=============" << endl;
		// 若房间列表不为空
		if (showReply->m_roomCount)
		{
			cout << "房间数量：" << showReply->m_roomCount << endl;
			for (int i = 0; i < showReply->m_roomCount; i++)
			{
				cout << "房间ID：" << showReply->GetRoomInfo()[i].m_roomID << "\t房间人数：" << showReply->GetRoomInfo()[i].m_currentMemberCount << "/" << showReply->GetRoomInfo()[i].m_maxMemberCount << endl;
			}
		}
		else
		{
			cout << "当前暂无房间" << endl;
		}
		cout << "========================================" << endl;
	}
	break;
	case MSG_ERROR:
	{
		MsgError* error = (MsgError*)msgBase;
		switch (error->getErrorCode())
		{
		case ERROR_CREATE:
			cout << "创建房间失败..." << endl;
			break;
		case ERROR_JOIN:
			cout << "加入房间失败..." << endl;
			break;
		case ERROR_NO_ENTER:
			cout << "还未进入聊天室，请加入聊天室后再进行发送消息或退出房间..." << endl;
			break;
		case ERROR_ALREADY_ENTER:
			cout << "当前已在聊天室中，请退出该房间后再进行创建或加入..." << endl;
			break;
		default:
			cout << "未知错误..." << endl;
		}
	}
	break;
	case MSG_CONN:
	{
		MsgConn* msgConn = (MsgConn*)msgBase;
		cout << "本用户id为[" << msgConn->getId() << "]" << endl;
		m_id = msgConn->getId();
	}
	break;
	default:
		cout << "消息解析失败！..." << endl;
		break;
	}
}

DWORD ChatRoomClient::ThreadProc(LPVOID lp)
{
	ChatRoomClient* client = (ChatRoomClient*)lp;
	client->PostRecv();
	return 0;
}




