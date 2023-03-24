#pragma warning(disable:4996)

#include "ChatRoomServer.h"

#include <Mswsock.h>
#include <string>
#pragma comment(lib, "Mswsock.lib")

/**
 * \brief �����ҹ���
 */
ChatRoom::ChatRoom(int id)
{
	// �洢����id
	m_ID = id;
	m_currentCount = 0;
	// ��ʼ���ٽ���Դ����
	InitializeCriticalSection(&m_secRoom);
}

/**
 * \brief ����������
 */
ChatRoom::~ChatRoom()
{
	// �ͷ��ٽ���Դ����
	DeleteCriticalSection(&m_secRoom);
}

/**
 * \brief ����������
 * \param id
 * \return
 */
int ChatRoom::Enter(ChatRoomServer* server, int id)
{
	EnterCriticalSection(&m_secRoom);
	// ��id�ſͻ��˼��뷿���Ա�б�
	m_memberVec.push_back(id);
	m_currentCount++;
	LeaveCriticalSection(&m_secRoom);

	// �㲥������Ϣ
	MsgBroad* msgBroad = new MsgBroad();
	std::string str = "�û�[" + std::to_string(id) + "]����[" + std::to_string(m_ID) + "]��������";
	strcpy(msgBroad->getBuff(), str.c_str());
	Broadcast(server, msgBroad);

	return 0;
}

/**
 * \brief
 * \param id �뿪������
 * \return
 */
int ChatRoom::Leave(ChatRoomServer* server, int id)
{
	EnterCriticalSection(&m_secRoom);
	// ��id�ſͻ��˼��뷿���Ա�б�
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
		// �㲥������Ϣ
		MsgBroad* msgBroad = new MsgBroad();
		std::string str = "�û�[" + std::to_string(id) + "]�뿪[" + std::to_string(m_ID) + "]��������";
		strcpy(msgBroad->getBuff(), str.c_str());
		Broadcast(server, msgBroad);
	}
	return 0;
}

/**
 * \brief �Է�����й㲥
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
 * \brief ����
 */
ChatRoomServer::ChatRoomServer()
{
	m_index = 0;
	m_roomCount = 0;
	m_indexRoom = 0;

	// ��ʼ���ٽ���Դ����
	InitializeCriticalSection(&m_sec);

	// ��ʼ����ɶ˿�
	m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// ��ȡҪ�������߳�����
	SYSTEM_INFO systemProcessorsCount;
	GetSystemInfo(&systemProcessorsCount);
	m_threadCount = systemProcessorsCount.dwNumberOfProcessors;
	//m_threadCount = 1;

	// �߳��б�����ռ�
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
 * \brief ����
 * \return
 */
int ChatRoomServer::StartUp()
{
	// ����socket
	//SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKET socketServer = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (socketServer == INVALID_SOCKET)
	{
		cout << "��������Socketʧ��..." << endl;
		return -1;
	}

	// ���ü�����ip�Ͷ˿�
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PORT);
	addrServer.sin_addr.s_addr = inet_addr(IP);

	// ��
	if (bind(socketServer, (const sockaddr*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR)
	{
		cout << "��ʧ��..." << endl;
		closesocket(socketServer);
		return 1;
	}

	// ����
	if (listen(socketServer, SOMAXCONN) == SOCKET_ERROR)
	{
		cout << "����ʧ��..." << endl;
		closesocket(socketServer);
		return -1;
	}
	else
	{
		cout << "��ʼ����..." << endl;
	}

	// ������������socket����Vec
	MyOverLapped* serverOv = new MyOverLapped(socketServer, m_index);
	m_mapID2OverLapped[m_index] = serverOv;
	m_index++;

	// ��������
	if (CreateIoCompletionPort(
		(HANDLE)socketServer,
		m_hCompletionPort,
		0,
		0
	) != m_hCompletionPort)
	{
		cout << "��ɶ˿ڰ�ʧ�ܣ�" << endl;
		closesocket(socketServer);
		return -1;
	}

	if (PostAccept() == true)
	{
		closesocket(socketServer);
		return -1;
	}

	// �����߳�
	for (int i = 0; i < m_threadCount; ++i)
	{
		m_pThread[i] = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
		if (NULL == m_pThread[i])
		{
			cout << "�����߳�ʧ�� ErrorCode:" << GetLastError() << endl;
			CloseHandle(m_hCompletionPort);
			closesocket(socketServer);
			return -1;
		}
	}

	// ��ֹ������ֹ
	WaitForMultipleObjects(m_threadCount, m_pThread, true, INFINITE);
}

/**
 * \brief ������Դ
 */
void ChatRoomServer::Clear()
{
	// �ͷ�socket��overlapped
	for (auto i = m_mapID2OverLapped.begin(); i != m_mapID2OverLapped.end(); i++)
	{
		closesocket(i->second->m_socket);
		WSACloseEvent(i->second->hEvent);
	}

	// ������
	for (auto i = m_mapID2Room.begin(); i != m_mapID2Room.end(); i++)
	{
		delete i->second;
		i->second = NULL;
		m_mapID2Room.erase(i);
	}

	// �ͷ��߳̾��
	for (int i = 0; i < m_threadCount; ++i)
	{
		CloseHandle(m_pThread[i]);
	}
	free(m_pThread);

	// �ͷ���ɶ˿�
	CloseHandle(m_hCompletionPort);

	// �ͷ��ٽ���Դ����
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
	// ����iocp
	if (CreateIoCompletionPort((HANDLE)socketClient, m_hCompletionPort, m_index, 0) == NULL)
	{
		cout << "��iocp����" << endl;
		return -1;
	}
	m_mapID2OverLapped[m_index] = clientMOL;

	char szBuff[MAXBYTE] = { 0 };
	DWORD dwRecved = 0;
	int len = sizeof(sockaddr) + 16;
	//��������
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
		return -1;	// ����ִ�г���
	}
	return 0;
}

/**
 * \brief Ͷ�ݽ���
 * \param id �ṹ���Ӧ��id
 * \return
 */
int ChatRoomServer::PostRecv(int id)
{
	DWORD dwFlag = 0;
	DWORD dwRecvCount;

	// ��������������λ�úʹ�С
	WSABUF* wsabuf = &m_mapID2OverLapped[id]->m_WSAbufRecv;
	wsabuf->buf = m_mapID2OverLapped[id]->m_buffRecv + m_mapID2OverLapped[id]->m_lastPos;
	wsabuf->len = MAX_BUFF_LEN - m_mapID2OverLapped[id]->m_lastPos;

	//�������ݵ�����
	WSARecv(m_mapID2OverLapped[id]->m_socket,
		&m_mapID2OverLapped[id]->m_WSAbufRecv,
		1,
		&dwRecvCount,
		&dwFlag,
		m_mapID2OverLapped[id],
		NULL);

	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		return -1;	// ����ִ�г���
	}
	return 0;
}

/**
 * \brief Ͷ�ݷ���
 * \param id �ͻ��˶�Ӧid
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
		return -1;	// ����ִ�г���
	}
	return 0;
}

/**
 * \brief ������Ϣ
 * \param id �ṹ���Ӧid
 * \param msgBase ��Ϣ
 */
void ChatRoomServer::DealWithMsg(int id, MsgBase* msgBase)
{
	switch (msgBase->m_msgType)
	{
	case MSG_SHWO:
	{
		cout << "�û�[" << id << "]����������" << endl;
		MsgShowReply msgShowReply;
		msgShowReply.m_roomCount = m_roomCount;
		int i = 0;
		// ����д�뷿����Ϣ
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
		// �Ƿ��ڷ�����
		if (m_mapID2OverLapped[id]->m_roomId)
		{
			// �ظ��ͻ���������������
			MsgError msgError(ERROR_ALREADY_ENTER);
			PostSend(id, &msgError);
		}
		else
		{
			MsgJoin* msgJoin = (MsgJoin*)msgBase;
			cout << "�û�[" << id << "]������� " << msgJoin->m_roomID << " ������" << endl;
			// �ж�Ӧ���䲢������û����
			if (m_mapID2Room[msgJoin->m_roomID] && m_mapID2Room[msgJoin->m_roomID]->m_currentCount < MAX_MEMBER_COUNT)
			{
				// ���ͻ��˼���������
				m_mapID2Room[msgJoin->m_roomID]->Enter(this, id);
				m_mapID2OverLapped[id]->m_roomId = msgJoin->m_roomID;
			}
			else
			{
				// ���ش�����
				MsgError errorMsg(ERROR_JOIN);
				PostSend(id, &errorMsg);
			}
		}
	}
	break;
	case MSG_CREATE:
	{
		// �Ƿ��ڷ�����
		if (m_mapID2OverLapped[id]->m_roomId)
		{
			// �ظ��ͻ���������������
			MsgError msgError(ERROR_ALREADY_ENTER);
			PostSend(id, &msgError);
		}
		else
		{
			if (m_roomCount < MAX_ROOM_COUNT)
			{
				cout << "�û�[" << id << "]���󴴽�������" << endl;
				m_indexRoom++;
				// ����������
				m_mapID2Room[m_indexRoom] = new ChatRoom(m_indexRoom);
				m_roomCount++;
				// ���ͻ��˼���������
				m_mapID2Room[m_indexRoom]->Enter(this, id);
				m_mapID2OverLapped[id]->m_roomId = m_indexRoom;
			}
			else
			{
				// �ظ��ͻ��˴�������ʧ��
				MsgError msgError(ERROR_CREATE);
				PostSend(id, &msgError);
			}
		}
	}
	break;
	case MSG_TALK:
	{
		MsgTalk* msgTalk = (MsgTalk*)msgBase;
		// �ڷ�����
		if (m_mapID2OverLapped[id]->m_roomId)
		{
			// �㲥����������Ϣ
			m_mapID2Room[m_mapID2OverLapped[id]->m_roomId]->Broadcast(this, msgTalk);
		}
		else
		{
			// �ظ��ͻ���δ����������
			MsgError msgError(ERROR_NO_ENTER);
			PostSend(id, &msgError);
		}
	}
	break;
	case MSG_LEAVE:
	{
		cout << "�û�[" << id << "]�뿪������..." << endl;
		// �ڷ�����
		if (m_mapID2OverLapped[id]->m_roomId)
		{
			m_mapID2Room[m_mapID2OverLapped[id]->m_roomId]->Leave(this, id);
			// ����������ԱΪ��������������
			if (m_mapID2Room[m_mapID2OverLapped[id]->m_roomId]->m_currentCount == 0)
			{
				// ��map���Ƴ���������
				EnterCriticalSection(&m_sec);
				delete m_mapID2Room[m_mapID2OverLapped[id]->m_roomId];
				m_mapID2Room.erase(m_mapID2OverLapped[id]->m_roomId);
				m_roomCount--;
				LeaveCriticalSection(&m_sec);
			}
			// �����û�������Ϣ���
			m_mapID2OverLapped[id]->m_roomId = 0;
		}
		else
		{
			// �ظ��ͻ���δ����������
			MsgError msgError(ERROR_NO_ENTER);
			PostSend(id, &msgError);
		}
	}
	break;
	default:
		cout << "��Ϣ����ʧ�ܣ������û�[" << id << "]..." << endl;
		break;
	}
}

/**
 * \brief �̺߳���
 * \param lp
 * \return
 */
DWORD ChatRoomServer::ThreadProc(LPVOID lp)
{
	ChatRoomServer* server = (ChatRoomServer*)lp;
	// ��Ӧ�ֽ���
	DWORD NumOfBytes;
	//������I/O��������ɵ��ļ������������ɼ�ֵ
	ULONG_PTR Key;
	LPOVERLAPPED lpOverlapped;

	while (true)
	{
		BOOL bFlag = GetQueuedCompletionStatus(server->m_hCompletionPort, &NumOfBytes, &Key, &lpOverlapped, INFINITE);
		if (bFlag == FALSE)
		{
			if (GetLastError() == 64)
			{
				std::cout << "�û�[" << Key << "]�Ͽ�" << std::endl;
				// ���ڷ��������˳�
				if (server->m_mapID2OverLapped[Key]->m_roomId)
				{
					server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId]->Leave(server, Key);

					// ����������ԱΪ��������������
					if (server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId]->m_currentCount == 0)
					{
						// ��map���Ƴ���������
						EnterCriticalSection(&server->m_sec);
						delete server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId];

						server->m_mapID2Room.erase(server->m_mapID2OverLapped[Key]->m_roomId);
						server->m_roomCount--;
						LeaveCriticalSection(&server->m_sec);
					}
				}
				// ������Դ
				closesocket(server->m_mapID2OverLapped[Key]->m_socket);
				WSACloseEvent(server->m_mapID2OverLapped[Key]->hEvent);
				// ��������ɾ��
				std::map<int, MyOverLapped*>::iterator iter = server->m_mapID2OverLapped.find(Key);
				server->m_mapID2OverLapped.erase(iter);
			}
			continue;
		}

		// ���д���
		// KeyΪ0 ���Ƿ���������socket������Ӧ
		if (Key == 0)
		{
			std::cout << "�û�[" << server->m_index << "]����" << std::endl;

			// ���ͻ��˷����û�id 
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
				std::cout << "�û�[" << Key << "]����" << std::endl;

				// ���ڷ��������˳�
				if (server->m_mapID2OverLapped[Key]->m_roomId)
				{
					server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId]->Leave(server, Key);

					// ����������ԱΪ��������������
					if (server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId]->m_currentCount == 0)
					{
						// ��map���Ƴ���������
						EnterCriticalSection(&server->m_sec);
						delete server->m_mapID2Room[server->m_mapID2OverLapped[Key]->m_roomId];

						server->m_mapID2Room.erase(server->m_mapID2OverLapped[Key]->m_roomId);
						server->m_roomCount--;
						LeaveCriticalSection(&server->m_sec);
					}
				}
				// ������Դ
				closesocket(server->m_mapID2OverLapped[Key]->m_socket);
				WSACloseEvent(server->m_mapID2OverLapped[Key]->hEvent);
				// ��������ɾ��
				std::map<int, MyOverLapped*>::iterator iter = server->m_mapID2OverLapped.find(Key);
				server->m_mapID2OverLapped.erase(iter);
			}
			else
			{
				// ��ȡ��Ӧ�ͻ���ip �˿�
				//std::cout << "ID: " << Key << "  ";
				MsgBase* phead = (MsgBase*)server->m_mapID2OverLapped[Key]->m_buffRecv;
				server->m_mapID2OverLapped[Key]->m_lastPos = server->m_mapID2OverLapped[Key]->m_lastPos + NumOfBytes;
				while (server->m_mapID2OverLapped[Key]->m_lastPos >= sizeof(MsgBase))
				{
					if (server->m_mapID2OverLapped[Key]->m_lastPos >= phead->m_dataLen)
					{
						// ���յ��ͻ�����Ϣ
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
				server->PostRecv(Key);	// ���Լ�Ͷ�ݽ���
			}
		}
	}
	return 0;
}

