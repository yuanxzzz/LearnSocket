#pragma warning(disable:4996)

#include "ChatRoomServer.h"

#include <Mswsock.h>
#include <string>
#pragma comment(lib, "Mswsock.lib")

/**
 * \brief 聊天室构造
 */
ChatRoom::ChatRoom(int id)
{
	// 存储房间id
	m_ID = id;
	m_currentCount = 0;
	// 初始化临界资源对象
	InitializeCriticalSection(&m_secRoom);
}

/**
 * \brief 聊天室析构
 */
ChatRoom::~ChatRoom()
{
	// 释放临界资源对象
	DeleteCriticalSection(&m_secRoom);
}

/**
 * \brief 进入聊天室
 * \param id
 * \return
 */
int ChatRoom::Enter(ChatRoomServer* server, int id)
{
	EnterCriticalSection(&m_secRoom);
	// 将id号客户端加入房间成员列表
	m_memberVec.push_back(id);
	m_currentCount++;
	LeaveCriticalSection(&m_secRoom);

	// 广播加入信息
	MsgBroad* msgBroad = new MsgBroad();
	std::string str = "用户[" + std::to_string(id) + "]加入[" + std::to_string(m_ID) + "]号聊天室";
	strcpy(msgBroad->getBuff(), str.c_str());
	Broadcast(server, msgBroad);

	return 0;
}

/**
 * \brief
 * \param id 离开聊天室
 * \return
 */
int ChatRoom::Leave(ChatRoomServer* server, int id)
{
	EnterCriticalSection(&m_secRoom);
	// 将id号客户端加入房间成员列表
	for (auto iter = m_memberVec.begin(); iter != m_memberVec.end(); iter++)
	{
		if (*iter == id)
		{
			m_memberVec.erase(iter);
			break;
		}
	}
	m_currentCount--;
	LeaveCriticalSection(&m_secRoom);

	if (m_currentCount)
	{
		// 广播加入信息
		MsgBroad* msgBroad = new MsgBroad();
		std::string str = "用户[" + std::to_string(id) + "]离开[" + std::to_string(m_ID) + "]号聊天室";
		strcpy(msgBroad->getBuff(), str.c_str());
		Broadcast(server, msgBroad);
	}
	return 0;
}

/**
 * \brief 对房间进行广播
 * \param server
 * \param msgBase
 */
void ChatRoom::Broadcast(ChatRoomServer* server, MsgBase* msgBase)
{
	for (int i = 0; i < m_currentCount; i++)
	{
		server->PostSend(m_memberVec[i], msgBase);
	}
}

/**
 * \brief 构造
 */
ChatRoomServer::ChatRoomServer()
{
	m_index = 0;
	m_roomCount = 0;
	m_indexRoom = 0;

	// 初始化临界资源对象
	InitializeCriticalSection(&m_sec);

	// 初始化完成端口
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// 获取要创建的线程数量
	SYSTEM_INFO systemProcessorsCount;
	GetSystemInfo(&systemProcessorsCount);
	m_threadCount = systemProcessorsCount.dwNumberOfProcessors;
	//m_threadCount = 1;

	// 线程列表申请空间
	m_pThread = (HANDLE*)malloc(sizeof(HANDLE) * m_threadCount);
}

/**
 * \brief
 */
ChatRoomServer::~ChatRoomServer()
{
	Clear();
}

/**
 * \brief 启动
 * \return
 */
int ChatRoomServer::StartUp()
{
	// 监听socket
	//SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKET socketServer = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (socketServer == INVALID_SOCKET)
	{
		cout << "创建监听Socket失败..." << endl;
		return -1;
	}

	// 设置监听的ip和端口
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PORT);
	addrServer.sin_addr.s_addr = inet_addr(IP);

	// 绑定
	if (bind(socketServer, (const sockaddr*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR)
	{
		cout << "绑定失败..." << endl;
		closesocket(socketServer);
		return 1;
	}

	// 监听
	if (listen(socketServer, SOMAXCONN) == SOCKET_ERROR)
	{
		cout << "监听失败..." << endl;
		closesocket(socketServer);
		return -1;
	}
	else
	{
		cout << "开始监听..." << endl;
	}

	// 将服务器监听socket加入Vec
	MyOverLapped* serverOv = new MyOverLapped(socketServer, m_index);
	m_mapID2OverLapped[m_index] = serverOv;
	m_index++;

	// 关联监听
	if (CreateIoCompletionPort(
		(HANDLE)socketServer,
		m_hCompletionPort,
		0,
		0
	) != m_hCompletionPort)
	{
		cout << "完成端口绑定失败！" << endl;
		closesocket(socketServer);
		return -1;
	}

	if (PostAccept() == true)
	{
		closesocket(socketServer);
		return -1;
	}

	// 创建线程
	for (int i = 0; i < m_threadCount; ++i)
	{
		m_pThread[i] = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
		if (NULL == m_pThread[i])
		{
			cout << "创建线程失败 ErrorCode:" << GetLastError() << endl;
			CloseHandle(m_hCompletionPort);
			closesocket(socketServer);
			return -1;
		}
	}

	// 防止进程终止
	WaitForMultipleObjects(m_threadCount, m_pThread, true, INFINITE);
}

/**
 * \brief 清理资源
 */
void ChatRoomServer::Clear()
{
	// 释放socket和overlapped
	for (auto i = m_mapID2OverLapped.begin(); i != m_mapID2OverLapped.end(); i++)
	{
		closesocket(i->second->m_socket);
		WSACloseEvent(i->second->hEvent);
	}

	// 清理房间
	for (auto i = m_mapID2Room.begin(); i != m_mapID2Room.end(); i++)
	{
		delete i->second;
		i->second = NULL;
		m_mapID2Room.erase(i);
	}

	// 释放线程句柄
	for (int i = 0; i < m_threadCount; ++i)
	{
		CloseHandle(m_pThread[i]);
	}
	free(m_pThread);

	// 释放完成端口
	CloseHandle(m_hCompletionPort);

	// 释放临界资源对象
	DeleteCriticalSection(&m_sec);
}

/**
 * \brief
 * \return
 */
int ChatRoomServer::PostAccept()
{
	SOCKET socketClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	MyOverLapped* clientMOL = new MyOverLapped(socketClient, m_index);
	// 关联iocp
	if (CreateIoCompletionPort((HANDLE)socketClient, m_hCompletionPort, m_index, 0) == NULL)
	{
		cout << "绑定iocp出错！" << endl;
		return -1;
	}
	m_mapID2OverLapped[m_index] = clientMOL;

	char szBuff[MAXBYTE] = { 0 };
	DWORD dwRecved = 0;
	int len = sizeof(sockaddr) + 16;
	//接收连接
	AcceptEx(m_mapID2OverLapped[0]->m_socket,
		socketClient,
		szBuff,
		0,
		sizeof(struct sockaddr_in) + 16,
		sizeof(struct sockaddr_in) + 16,
		&dwRecved,
		clientMOL);

	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		return -1;	// 函数执行出错
	}
	return 0;
}

/**
 * \brief 投递接收
 * \param id 结构体对应的id
 * \return
 */
int ChatRoomServer::PostRecv(int id)
{
	DWORD dwFlag = 0;
	DWORD dwRecvCount;

	// 调整缓冲区接收位置和大小
	WSABUF* wsabuf = &m_mapID2OverLapped[id]->m_WSAbufRecv;
	wsabuf->buf = m_mapID2OverLapped[id]->m_buffRecv + m_mapID2OverLapped[id]->m_lastPos;
	wsabuf->len = MAX_BUFF_LEN - m_mapID2OverLapped[id]->m_lastPos;

	//接收数据的请求
	WSARecv(m_mapID2OverLapped[id]->m_socket,
		&m_mapID2OverLapped[id]->m_WSAbufRecv,
		1,
		&dwRecvCount,
		&dwFlag,
		m_mapID2OverLapped[id],
		NULL);

	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		return -1;	// 函数执行出错
	}
	return 0;
}

/**
 * \brief 投递发送
 * \param id 客户端对应id
 * \return
 */
int i = 0;
int ChatRoomServer::PostSend(int id, MsgBase* msgBase)
{
	DWORD dwFlag = 0;
	DWORD dwSendCount;

	/*WSASend(m_mapID2OverLapped[id]->m_socket,
		&m_mapID2OverLapped[id]->m_WSAbufSend,
		1,
		&dwSendCount,
		dwFlag,
		m_mapID2OverLapped[id],
		NULL);*/
		//send(m_mapID2OverLapped[id]->m_socket, m_mapID2OverLapped[id]->m_buffSend, msgBase->m_dataLen, NULL);
	send(m_mapID2OverLapped[id]->m_socket, (const char*)msgBase, msgBase->m_dataLen, NULL);

	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		return -1;	// 函数执行出错
	}
	return 0;
}

/**
 * \brief 处理消息
 * \param id 结构体对应id
 * \param msgBase 消息
 */
void ChatRoomServer::DealWithMsg(int id, MsgBase* msgBase)
{
	switch (msgBase->m_msgType)
	{
	case MSG_SHWO:
	{
		cout << "用户[" << id << "]请求聊天室" << endl;
		MsgShowReply msgShowReply;
		msgShowReply.m_roomCount = m_roomCount;
		int i = 0;
		// 依次写入房间信息
		for (auto iter = m_mapID2Room.begin(); iter != m_mapID2Room.end(); iter++, i++)
		{
			msgShowReply.GetRoomInfo()[i].m_roomID = iter->first;
			msgShowReply.GetRoomInfo()[i].m_currentMemberCount = iter->second->m_currentCount;
			msgShowReply.GetRoomInfo()[i].m_maxMemberCount = MAX_MEMBER_COUNT;
		}
		PostSend(id, &msgShowReply);
	}
	break;
	case MSG_JOIN:
	{
		// 是否在房间中
		if (m_mapID2OverLapped[id]->m_roomId)
		{
			// 回复客户端已在聊天室中
			MsgError msgError(ERROR_ALREADY_ENTER);
			PostSend(id, &msgError);
		}
		else
		{
			MsgJoin* msgJoin = (MsgJoin*)msgBase;
			cout << "用户[" << id << "]请求加入 " << msgJoin->m_roomID << " 聊天室" << endl;
			// 有对应房间并且人数没有满
			if (m_mapID2Room[msgJoin->m_roomID] && m_mapID2Room[msgJoin->m_roomID]->m_currentCount < MAX_MEMBER_COUNT)
			{
				// 将客户端加入聊天室
				m_mapID2Room[msgJoin->m_roomID]->Enter(this, id);
				m_mapID2OverLapped[id]->m_roomId = msgJoin->m_roomID;
			}
			else
			{
				// 返回错误码
				MsgError errorMsg(ERROR_JOIN);
				PostSend(id, &errorMsg);
			}
		}
	}
	break;
	case MSG_CREATE:
	{
		// 是否在房间中
		if (m_mapID2OverLapped[id]->m_roomId)
		{
			// 回复客户端已在聊天室中
			MsgError msgError(ERROR_ALREADY_ENTER);
			PostSend(id, &msgError);
		}
		else
		{
			if (m_roomCount < MAX_ROOM_COUNT)
			{
				cout << "用户[" << id << "]请求创建聊天室" << endl;
				m_indexRoom++;
				// 创建聊天室
				m_mapID2Room[m_indexRoom] = new ChatRoom(m_indexRoom);
				m_roomCount++;
				// 将客户端加入聊天室
				m_mapID2Room[m_indexRoom]->Enter(this, id);
				m_mapID2OverLapped[id]->m_roomId = m_indexRoom;
			}
			else
			{
				// 回复客户端创建房间失败
				MsgError msgError(ERROR_CREATE);
				PostSend(id, &msgError);
			}
		}
	}
	break;
	case MSG_TALK:
	{
		MsgTalk* msgTalk = (MsgTalk*)msgBase;
		// 在房间中
		if (m_mapID2OverLapped[id]->m_roomId)
		{
			// 广播该条聊天消息
			m_mapID2Room[m_mapID2OverLapped[id]->m_roomId]->Broadcast(this, msgTalk);
		}
		else
		{
			// 回复客户端未进入聊天室
			MsgError msgError(ERROR_NO_ENTER);
			PostSend(id, &msgError);
		}
	}
	break;
	case MSG_LEAVE:
	{
		cout << "用户[" << id << "]离开聊天室..." << endl;
		// 在房间中
		if (m_mapID2OverLapped[id]->m_roomId)
		{
			m_mapID2Room[m_mapID2OverLapped[id]->m_roomId]->Leave(this, id);
			// 若聊天室人员为空则销毁聊天室
			if (m_mapID2Room[m_mapID2OverLapped[id]->m_roomId]->m_currentCount == 0)
			{
				// 从map中移除并清理房间
				EnterCriticalSection(&m_sec);
				delete m_mapID2Room[m_mapID2OverLapped[id]->m_roomId];
				m_mapID2Room.erase(m_mapID2OverLapped[id]->m_roomId);
				m_roomCount--;
				LeaveCriticalSection(&m_sec);
			}
			// 将该用户房间信息清空
			m_mapID2OverLapped[id]->m_roomId = 0;
		}
		else
		{
			// 回复客户端未进入聊天室
			MsgError msgError(ERROR_NO_ENTER);
			PostSend(id, &msgError);
		}
	}
	break;
	default:
		cout << "消息解析失败！来自用户[" << id << "]..." << endl;
		break;
	}
}

/**
 * \brief 线程函数
 * \param lp
 * \return
 */
DWORD ChatRoomServer::ThreadProc(LPVOID lp)
{
	ChatRoomServer* server = (ChatRoomServer*)lp;
	// 对应字节数
	DWORD NumOfBytes;
	//接收与I/O操作已完成的文件句柄关联的完成键值
	ULONG_PTR Key;
	LPOVERLAPPED lpOverlapped;

	while (true)
	{
		BOOL bFlag = GetQueuedCompletionStatus(server->m_hCompletionPort, &NumOfBytes, &Key, &lpOverlapped, INFINITE);
		if (bFlag == FALSE)
		{
			if (GetLastError() == 64)
			{
				std::cout << "用户[" << Key << "]断开" << std::endl;
				// 若在房间中则退出
				if (server->m_mapID2OverLapped[Key]->m_roomId)
				{
					server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId]->Leave(server, Key);

					// 若聊天室人员为空则销毁聊天室
					if (server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId]->m_currentCount == 0)
					{
						// 从map中移除并清理房间
						EnterCriticalSection(&server->m_sec);
						delete server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId];

						server->m_mapID2Room.erase(server->m_mapID2OverLapped[Key]->m_roomId);
						server->m_roomCount--;
						LeaveCriticalSection(&server->m_sec);
					}
				}
				// 清理资源
				closesocket(server->m_mapID2OverLapped[Key]->m_socket);
				WSACloseEvent(server->m_mapID2OverLapped[Key]->hEvent);
				// 从容器中删掉
				std::map<int, MyOverLapped*>::iterator iter = server->m_mapID2OverLapped.find(Key);
				server->m_mapID2OverLapped.erase(iter);
			}
			continue;
		}

		// 进行处理
		// Key为0 则是服务器监听socket进行响应
		if (Key == 0)
		{
			std::cout << "用户[" << server->m_index << "]上线" << std::endl;

			// 给客户端发送用户id 
			MsgConn msgConn(server->m_index);
			server->PostSend(server->m_index, &msgConn);

			server->PostRecv(server->m_index);
			server->m_index++;
			server->PostAccept();
		}
		else
		{
			if (0 == NumOfBytes)
			{
				std::cout << "用户[" << Key << "]下线" << std::endl;

				// 若在房间中则退出
				if (server->m_mapID2OverLapped[Key]->m_roomId)
				{
					server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId]->Leave(server, Key);

					// 若聊天室人员为空则销毁聊天室
					if (server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId]->m_currentCount == 0)
					{
						// 从map中移除并清理房间
						EnterCriticalSection(&server->m_sec);
						delete server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId];

						server->m_mapID2Room.erase(server->m_mapID2OverLapped[Key]->m_roomId);
						server->m_roomCount--;
						LeaveCriticalSection(&server->m_sec);
					}
				}
				// 清理资源
				closesocket(server->m_mapID2OverLapped[Key]->m_socket);
				WSACloseEvent(server->m_mapID2OverLapped[Key]->hEvent);
				// 从容器中删掉
				std::map<int, MyOverLapped*>::iterator iter = server->m_mapID2OverLapped.find(Key);
				server->m_mapID2OverLapped.erase(iter);
			}
			else
			{
				// 获取对应客户端ip 端口
				//std::cout << "ID: " << Key << "  ";
				MsgBase* phead = (MsgBase*)server->m_mapID2OverLapped[Key]->m_buffRecv;
				server->m_mapID2OverLapped[Key]->m_lastPos = server->m_mapID2OverLapped[Key]->m_lastPos + NumOfBytes;
				while (server->m_mapID2OverLapped[Key]->m_lastPos >= sizeof(MsgBase))
				{
					if (server->m_mapID2OverLapped[Key]->m_lastPos >= phead->m_dataLen)
					{
						// 接收到客户端消息
						//std::cout << ((MsgTalk*)phead)->GetBuff() << std::endl;
						server->DealWithMsg(Key, phead);
						memcpy(server->m_mapID2OverLapped[Key]->m_buffRecv, server->m_mapID2OverLapped[Key]->m_buffRecv + phead->m_dataLen, server->m_mapID2OverLapped[Key]->m_lastPos - phead->m_dataLen);
						server->m_mapID2OverLapped[Key]->m_lastPos -= phead->m_dataLen;
					}
					else
					{
						break;
					}
				}
				server->PostRecv(Key);	// 对自己投递接收
			}
		}
	}
	return 0;
}

