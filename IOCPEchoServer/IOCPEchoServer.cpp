#pragma warning(disable:4996)

#include <iostream>
#include <map>

#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"
#include <Mswsock.h>
#include <vector>
#include <WS2tcpip.h>
#pragma comment(lib, "Mswsock.lib")

#define MAX_BUFF_LEN 1024

// ����һ���Զ���� OVERLAPPED �Ľṹ������һЩ����
struct MyOverLapped :public OVERLAPPED
{
	// ����
	MyOverLapped(SOCKET sock, int ID)
	{
		memset(this, 0, sizeof(MyOverLapped));
		m_socket = sock;
		m_WSAbuf.buf = m_buff;
		m_WSAbuf.len = sizeof(m_buff);
		m_ID = ID;
		m_lastPos = 0;
	}

	// ��Ӧ��socket
	SOCKET m_socket;

	// ����
	char m_buff[MAX_BUFF_LEN * 2];

	// �Լ�����µ�����
	WSABUF m_WSAbuf;

	// ��ӦID
	int m_ID;

	// ��һ�ν�������λ��
	int m_lastPos;
};

/**
 * \brief �Ƿ�����
 */
bool g_isRunning = true;

/**
 * \brief socket��ID
 */
int g_index = 0;

/**
 * \brief ���ڱ����������ӵĿͻ���
 */
std::map<int, MyOverLapped*> g_mapMOL;

/**
 * \brief 
 * \param lp �̺߳���
 * \return 
 */
DWORD WINAPI ThreadProc(LPVOID lp);

/**
 * \brief ��������
 * \param hiocp 
 * \return 
 */
int PostAccept(HANDLE hiocp);

/**
 * \brief 
 * \param index ������Ϣ
 * \return 
 */
int PostRecv(int index);

int main()
{
	// ��ʼ��socket��
	SocketInit socketInit;

	// ��ʼ��IOCP
	HANDLE completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (completionPort == NULL) {
		std::cout << "������ɶ˿�ʧ��...\n" << std::endl;
		return -1;
	}

	// ����socket
	//SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKET socketServer = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (socketServer == INVALID_SOCKET)
	{
		std::cout << "��������Socketʧ��...\n";
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
		std::cout << "��ʧ��...\n";
		closesocket(socketServer);
		return 1;
	}

	// ����
	if (listen(socketServer, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "����ʧ��...\n";
		closesocket(socketServer);
		return -1;
	}
	else
	{
		std::cout << "��ʼ����...\n";
	}

	// ������������socket����Vec
	MyOverLapped* serverOv = new MyOverLapped(socketServer, g_index);
	g_mapMOL[g_index] = serverOv;
	g_index++;

	// ����IOCP����
	HANDLE hIocp = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,
		NULL,
		NULL,
		0
	);
	if (hIocp == NULL)
	{
		std::cout << "��ɶ˿ڴ���ʧ�ܣ�" << std::endl;
		closesocket(socketServer);
		return -1;
	}

	// ��������
	HANDLE hIocp1 = CreateIoCompletionPort(
		(HANDLE)socketServer,
		hIocp,
		0,
		0
	);
	if (hIocp1 != hIocp)
	{
		std::cout << "��ɶ˿ڰ�ʧ�ܣ�" << std::endl;
		closesocket(socketServer);
		return -1;
	}

	// ��ȡҪ�������߳�����
	SYSTEM_INFO systemProcessorsCount;
	GetSystemInfo(&systemProcessorsCount);
	int nProcessorsCount = systemProcessorsCount.dwNumberOfProcessors;
	//int nProcessorsCount = 1;

	//3��Ͷ��һ���������ӵ�����
	if (PostAccept(hIocp) == true)
	{
		closesocket(socketServer);
		return -1;
	}

	// �����߳�
	HANDLE* pThread = (HANDLE*)malloc(sizeof(HANDLE) * nProcessorsCount);
	for (int i = 0; i < nProcessorsCount; ++i)
	{
		pThread[i] = CreateThread(NULL, 0, ThreadProc, hIocp, 0, NULL);
		if (NULL == pThread[i])
		{
			std::cout << "�����߳�ʧ�� ErrorCode:" << GetLastError() << std::endl;
			CloseHandle(hIocp);
			closesocket(socketServer);
			return -1;
		}
	}

	// ����
	while (true)
	{
		Sleep(1000);
	}

	// �ͷ��߳̾��
	for (int i = 0; i < nProcessorsCount; ++i)
	{
		CloseHandle(pThread[i]);
	}
	free(pThread);

	CloseHandle(hIocp);

	return 0;
}

DWORD ThreadProc(LPVOID lp)
{
	HANDLE port = (HANDLE)lp;
	// ��Ӧ�ֽ���
	DWORD NumOfBytes;
	//������I/O��������ɵ��ļ������������ɼ�ֵ
	ULONG_PTR Key;
	LPOVERLAPPED lpOverlapped;

	while (g_isRunning)
	{
		BOOL bFlag = GetQueuedCompletionStatus(port, &NumOfBytes, &Key, &lpOverlapped, INFINITE);
		if (bFlag == FALSE)
		{
			if (GetLastError() == 64)
			{
				std::cout << "�ͻ���" << "ID: " << Key << "�Ͽ�" << std::endl;
			}
			continue;
		}

		// ���д���
		// KeyΪ0 ���Ƿ���������socket������Ӧ
		if (Key == 0)
		{
			std::cout << "�ͻ���" << "ID: " << g_index << "����" << std::endl;
			PostRecv(g_index);
			g_index++;
			PostAccept(port);
		}
		else
		{
			if (0 == NumOfBytes)
			{
				std::cout << "�ͻ���" << "ID: " << Key << "����" << std::endl;
				closesocket(g_mapMOL[Key]->m_socket);
				WSACloseEvent(g_mapMOL[Key]->hEvent);
				// ��������ɾ��
				std::map<int, MyOverLapped*>::iterator iter = g_mapMOL.find(Key);
				g_mapMOL.erase(iter);
			}
			else
			{
				// ��ȡ��Ӧ�ͻ���ip �˿�
				std::cout << "ID: " << Key << "  ";
				MsgBase* phead = (MsgBase*)g_mapMOL[Key]->m_buff;
				g_mapMOL[Key]->m_lastPos = g_mapMOL[Key]->m_lastPos + NumOfBytes;
				while (g_mapMOL[Key]->m_lastPos >= sizeof(MsgBase))
				{
					if (g_mapMOL[Key]->m_lastPos >= phead->m_dataLen)
					{
						// ���յ��ͻ�����Ϣ
						std::cout << ((MsgTalk*)phead)->GetBuff() << std::endl;
						memcpy(g_mapMOL[Key]->m_buff, g_mapMOL[Key]->m_buff + phead->m_dataLen, g_mapMOL[Key]->m_lastPos - phead->m_dataLen);
						g_mapMOL[Key]->m_lastPos -= phead->m_dataLen;
					}
					else
					{
						break;
					}
				}
				PostRecv(Key);	// ���Լ�Ͷ�ݽ���
			}
		}
	}
	return 0;
}


/**
 * \brief �첽��������
 * \param hIocp 
 * \return 
 */
int PostAccept(HANDLE hIocp)
{
	SOCKET socketClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	MyOverLapped* clientMOL = new MyOverLapped(socketClient, g_index);
	// ����iocp
	if (CreateIoCompletionPort((HANDLE)socketClient, hIocp, g_index, 0) == NULL)
	{
		std::cout << "��iocp����" << std::endl;
		return -1;
	}
	g_mapMOL[g_index] = clientMOL;

	char szBuff[MAXBYTE] = { 0 };
	DWORD dwRecved = 0;
	int len = sizeof(sockaddr) + 16;
	//��������
	AcceptEx(g_mapMOL[0]->m_socket,
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
 * \brief �첽������Ϣ
 * \param ID ��Ӧ�ṹ��id
 * \return 
 */
int PostRecv(int ID)
{
	DWORD dwFlag = 0;
	DWORD dwRecvCount;

	// ��������������λ�úʹ�С
	WSABUF* wsabuf = &g_mapMOL[ID]->m_WSAbuf;
	wsabuf->buf = g_mapMOL[ID]->m_buff + g_mapMOL[ID]->m_lastPos;
	wsabuf->len = MAX_BUFF_LEN - g_mapMOL[ID]->m_lastPos;

	//�������ݵ�����
	WSARecv(g_mapMOL[ID]->m_socket,
		&g_mapMOL[ID]->m_WSAbuf,
		1,
		&dwRecvCount,
		&dwFlag,
		g_mapMOL[ID],
		NULL);

	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		return -1;	// ����ִ�г���
	}
	return 0;
}
