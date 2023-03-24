#pragma warning(disable:4996)

#include "ChatRoomClient.h"

ChatRoomClient::ChatRoomClient()
{
	m_lastPos = 0;
	m_isRunnning = true;
}

ChatRoomClient::~ChatRoomClient()
{
	cout << "�رտͻ���" << endl;
	shutdown(m_socket, 2);
	Clear();
}

/**
 * \brief ������Դ
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
 * \brief ����
 * \return 
 */
int ChatRoomClient::Run()
{
	// ��ʼ��socket
	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// ����ip�Ͷ˿�
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PORT);
	addrServer.sin_addr.s_addr = inet_addr(IP);

	if (connect(m_socket, (sockaddr*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR)
	{
		cout << "���ӷ�����ʧ��..." << endl;
		closesocket(m_socket);
		return -1;
	}
	else
	{
		cout << "���ӷ������ɹ�..." << endl;
	}

	// �����߳̽��н�����Ϣ
	m_hThread = CreateThread(NULL, NULL, ThreadProc, this, NULL, NULL);

	// ���߳̽��з���
	PostSend();
}

/**
 * \brief ����
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
			// ѭ�����
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
			// ���ճ���
			cout << "������Ϣ�����Ͽ�����!" << endl;
			m_isRunnning = false;
			return -1;
		}
	}
	return 0;
}

/**
 * \brief ����
 * \return 
 */
int ChatRoomClient::PostSend()
{
	cout << "��ʾ�����б�\t���뷿��\t��������\t��������\t�˳�������\t�˳��ͻ���" << endl;
	cout << "show\t\tjoin\t\tcreate\t\ttalk\t\tleave\t\texit" << endl;

	while (m_isRunnning)
	{
		m_buffSend[MAX_BUFF_LEN] = { 0 };
		cin >> m_buffSend;
		// ��ʾ������
		if (strcmp(m_buffSend, "show") == 0)
		{
			MsgShow msgShow;
			SendMsg(&msgShow);
		}
		// ����������
		else if (strncmp(m_buffSend, "join", strlen("join")) == 0)
		{
			// ��ȡ�����
			int r = atoi(m_buffSend + strlen("join"));
			MsgJoin msgJoin(r);
			SendMsg(&msgJoin);
		}
		// ����������
		else if (strcmp(m_buffSend, "create") == 0)
		{
			MsgCreate msgCreate;
			SendMsg(&msgCreate);
		}
		// ����
		else if (strcmp(m_buffSend, "talk") == 0)
		{
			cout << "��ʼ���죨���� leave �˳�����)" << endl;
			while (true)
			{
				MsgTalk msgTalk;
				msgTalk.SetId(m_id);
				//gets_s(msgTalk.GetBuff(), 1000);
				cin >> msgTalk.GetBuff();
				if (strcmp(msgTalk.GetBuff(), "leave") == 0) {
					cout << "�������죬�˳�������" << endl;
					MsgLeave msgLeave;
					SendMsg(&msgLeave);
					break;
				}
				// ��Ϊ�ղŷ���
				if(msgTalk.GetBuff()[0] != '\0')
				{
					SendMsg(&msgTalk);
				}
			}
		}
		// �˳�������
		else if (strcmp(m_buffSend, "leave") == 0)
		{
			cout << "�������죬�˳�������" << endl;
			MsgLeave msgLeave;
			SendMsg(&msgLeave);
		}
		// �˳�����
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
 * \brief ������Ϣ
 * \param msgBase ��Ϣָ�� 
 */
void ChatRoomClient::DealWithMsg(MsgBase* msgBase)
{
	switch (msgBase->m_msgType)
	{
	case MSG_SHWO:
		cout << "��ʾ����������" << endl;
		break;
	case MSG_JOIN:
	{
		MsgJoin* join = (MsgJoin*)msgBase;
		cout << "������� " << join->m_roomID << " ������" << endl;
	}
	break;
	case MSG_CREATE:
		cout << "���󴴽�������" << endl;
		break;
	case MSG_TALK:
	{
		MsgTalk* msgTalk = (MsgTalk*)msgBase;
		if (msgTalk->GetId() != m_id)
		{
			cout << "�û�ID[" << msgTalk->GetId() << "]��" << msgTalk->GetBuff() << endl;
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
		cout << "�뿪������..." << endl;
		break;
	}
	case MSG_SHOWREPLY:
	{
		MsgShowReply* showReply = (MsgShowReply*)msgBase;
		cout << "=============չʾ��������Ϣ=============" << endl;
		// �������б�Ϊ��
		if (showReply->m_roomCount)
		{
			cout << "����������" << showReply->m_roomCount << endl;
			for (int i = 0; i < showReply->m_roomCount; i++)
			{
				cout << "����ID��" << showReply->GetRoomInfo()[i].m_roomID << "\t����������" << showReply->GetRoomInfo()[i].m_currentMemberCount << "/" << showReply->GetRoomInfo()[i].m_maxMemberCount << endl;
			}
		}
		else
		{
			cout << "��ǰ���޷���" << endl;
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
			cout << "��������ʧ��..." << endl;
			break;
		case ERROR_JOIN:
			cout << "���뷿��ʧ��..." << endl;
			break;
		case ERROR_NO_ENTER:
			cout << "��δ���������ң�����������Һ��ٽ��з�����Ϣ���˳�����..." << endl;
			break;
		case ERROR_ALREADY_ENTER:
			cout << "��ǰ�����������У����˳��÷�����ٽ��д��������..." << endl;
			break;
		default:
			cout << "δ֪����..." << endl;
		}
	}
	break;
	case MSG_CONN:
	{
		MsgConn* msgConn = (MsgConn*)msgBase;
		cout << "���û�idΪ[" << msgConn->getId() << "]" << endl;
		m_id = msgConn->getId();
	}
	break;
	default:
		cout << "��Ϣ����ʧ�ܣ�..." << endl;
		break;
	}
}

DWORD ChatRoomClient::ThreadProc(LPVOID lp)
{
	ChatRoomClient* client = (ChatRoomClient*)lp;
	client->PostRecv();
	return 0;
}




