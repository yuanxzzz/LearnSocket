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

// 创建一个自定义的 OVERLAPPED 的结构，附加一些数据
struct MyOverLapped :public OVERLAPPED
{
	// 构造
	MyOverLapped(SOCKET sock, int ID)
	{
		memset(this, 0, sizeof(MyOverLapped));
		m_socket = sock;
		m_WSAbuf.buf = m_buff;
		m_WSAbuf.len = sizeof(m_buff);
		m_ID = ID;
		m_lastPos = 0;
	}

	// 对应的socket
	SOCKET m_socket;

	// 缓存
	char m_buff[MAX_BUFF_LEN * 2];

	// 自己添加新的数据
	WSABUF m_WSAbuf;

	// 对应ID
	int m_ID;

	// 上一次解析到的位置
	int m_lastPos;
};

/**
 * \brief 是否运行
 */
bool g_isRunning = true;

/**
 * \brief socket的ID
 */
int g_index = 0;

/**
 * \brief 用于保存所有连接的客户端
 */
std::map<int, MyOverLapped*> g_mapMOL;

/**
 * \brief 
 * \param lp 线程函数
 * \return 
 */
DWORD WINAPI ThreadProc(LPVOID lp);

/**
 * \brief 接收连接
 * \param hiocp 
 * \return 
 */
int PostAccept(HANDLE hiocp);

/**
 * \brief 
 * \param index 接收消息
 * \return 
 */
int PostRecv(int index);

int main()
{
	// 初始化socket库
	SocketInit socketInit;

	// 初始化IOCP
	HANDLE completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (completionPort == NULL) {
		std::cout << "创建完成端口失败...\n" << std::endl;
		return -1;
	}

	// 监听socket
	//SOCKET socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKET socketServer = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (socketServer == INVALID_SOCKET)
	{
		std::cout << "创建监听Socket失败...\n";
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
		std::cout << "绑定失败...\n";
		closesocket(socketServer);
		return 1;
	}

	// 监听
	if (listen(socketServer, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "监听失败...\n";
		closesocket(socketServer);
		return -1;
	}
	else
	{
		std::cout << "开始监听...\n";
	}

	// 将服务器监听socket加入Vec
	MyOverLapped* serverOv = new MyOverLapped(socketServer, g_index);
	g_mapMOL[g_index] = serverOv;
	g_index++;

	// 创建IOCP对象
	HANDLE hIocp = CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,
		NULL,
		NULL,
		0
	);
	if (hIocp == NULL)
	{
		std::cout << "完成端口创建失败！" << std::endl;
		closesocket(socketServer);
		return -1;
	}

	// 关联监听
	HANDLE hIocp1 = CreateIoCompletionPort(
		(HANDLE)socketServer,
		hIocp,
		0,
		0
	);
	if (hIocp1 != hIocp)
	{
		std::cout << "完成端口绑定失败！" << std::endl;
		closesocket(socketServer);
		return -1;
	}

	// 获取要创建的线程数量
	SYSTEM_INFO systemProcessorsCount;
	GetSystemInfo(&systemProcessorsCount);
	int nProcessorsCount = systemProcessorsCount.dwNumberOfProcessors;
	//int nProcessorsCount = 1;

	//3）投递一个接收连接的请求
	if (PostAccept(hIocp) == true)
	{
		closesocket(socketServer);
		return -1;
	}

	// 创建线程
	HANDLE* pThread = (HANDLE*)malloc(sizeof(HANDLE) * nProcessorsCount);
	for (int i = 0; i < nProcessorsCount; ++i)
	{
		pThread[i] = CreateThread(NULL, 0, ThreadProc, hIocp, 0, NULL);
		if (NULL == pThread[i])
		{
			std::cout << "创建线程失败 ErrorCode:" << GetLastError() << std::endl;
			CloseHandle(hIocp);
			closesocket(socketServer);
			return -1;
		}
	}

	// 阻塞
	while (true)
	{
		Sleep(1000);
	}

	// 释放线程句柄
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
	// 对应字节数
	DWORD NumOfBytes;
	//接收与I/O操作已完成的文件句柄关联的完成键值
	ULONG_PTR Key;
	LPOVERLAPPED lpOverlapped;

	while (g_isRunning)
	{
		BOOL bFlag = GetQueuedCompletionStatus(port, &NumOfBytes, &Key, &lpOverlapped, INFINITE);
		if (bFlag == FALSE)
		{
			if (GetLastError() == 64)
			{
				std::cout << "客户端" << "ID: " << Key << "断开" << std::endl;
			}
			continue;
		}

		// 进行处理
		// Key为0 则是服务器监听socket进行响应
		if (Key == 0)
		{
			std::cout << "客户端" << "ID: " << g_index << "上线" << std::endl;
			PostRecv(g_index);
			g_index++;
			PostAccept(port);
		}
		else
		{
			if (0 == NumOfBytes)
			{
				std::cout << "客户端" << "ID: " << Key << "下线" << std::endl;
				closesocket(g_mapMOL[Key]->m_socket);
				WSACloseEvent(g_mapMOL[Key]->hEvent);
				// 从容器中删掉
				std::map<int, MyOverLapped*>::iterator iter = g_mapMOL.find(Key);
				g_mapMOL.erase(iter);
			}
			else
			{
				// 获取对应客户端ip 端口
				std::cout << "ID: " << Key << "  ";
				MsgBase* phead = (MsgBase*)g_mapMOL[Key]->m_buff;
				g_mapMOL[Key]->m_lastPos = g_mapMOL[Key]->m_lastPos + NumOfBytes;
				while (g_mapMOL[Key]->m_lastPos >= sizeof(MsgBase))
				{
					if (g_mapMOL[Key]->m_lastPos >= phead->m_dataLen)
					{
						// 接收到客户端消息
						std::cout << ((MsgTalk*)phead)->GetBuff() << std::endl;
						memcpy(g_mapMOL[Key]->m_buff, g_mapMOL[Key]->m_buff + phead->m_dataLen, g_mapMOL[Key]->m_lastPos - phead->m_dataLen);
						g_mapMOL[Key]->m_lastPos -= phead->m_dataLen;
					}
					else
					{
						break;
					}
				}
				PostRecv(Key);	// 对自己投递接收
			}
		}
	}
	return 0;
}


/**
 * \brief 异步接受连接
 * \param hIocp 
 * \return 
 */
int PostAccept(HANDLE hIocp)
{
	SOCKET socketClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	MyOverLapped* clientMOL = new MyOverLapped(socketClient, g_index);
	// 关联iocp
	if (CreateIoCompletionPort((HANDLE)socketClient, hIocp, g_index, 0) == NULL)
	{
		std::cout << "绑定iocp出错！" << std::endl;
		return -1;
	}
	g_mapMOL[g_index] = clientMOL;

	char szBuff[MAXBYTE] = { 0 };
	DWORD dwRecved = 0;
	int len = sizeof(sockaddr) + 16;
	//接收连接
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
		return -1;	// 函数执行出错
	}
	return 0;
}

/**
 * \brief 异步接收消息
 * \param ID 对应结构体id
 * \return 
 */
int PostRecv(int ID)
{
	DWORD dwFlag = 0;
	DWORD dwRecvCount;

	// 调整缓冲区接收位置和大小
	WSABUF* wsabuf = &g_mapMOL[ID]->m_WSAbuf;
	wsabuf->buf = g_mapMOL[ID]->m_buff + g_mapMOL[ID]->m_lastPos;
	wsabuf->len = MAX_BUFF_LEN - g_mapMOL[ID]->m_lastPos;

	//接收数据的请求
	WSARecv(g_mapMOL[ID]->m_socket,
		&g_mapMOL[ID]->m_WSAbuf,
		1,
		&dwRecvCount,
		&dwFlag,
		g_mapMOL[ID],
		NULL);

	if (ERROR_IO_PENDING != WSAGetLastError())
	{
		return -1;	// 函数执行出错
	}
	return 0;
}
