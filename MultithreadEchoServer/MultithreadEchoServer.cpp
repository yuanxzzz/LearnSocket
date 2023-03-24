#pragma warning(disable:4996)

#include <iostream>
#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"

/**
 * \brief 处理socket连接及接收数据
 * \param lp 
 * \return 
 */
DWORD WINAPI ThreadProc(LPVOID lp)
{
	SOCKET clientSocket = *(SOCKET*)lp;

	// 获取对应客户端ip 端口
	sockaddr_in addr;
	int len = sizeof(addr);
	getpeername(clientSocket, (sockaddr*)&addr, &len);

	char recvBuff[1024] = { 0 };
	int lastPos = 0;
	while (true)
	{
		int recvCount = recv(clientSocket, recvBuff + lastPos, PACKET_MAX_SIZE - lastPos, NULL);
		if (recvCount > 0)
		{
			MsgBase* phead = (MsgBase*)recvBuff;
			lastPos = lastPos + recvCount;
			// 循环拆包
			// lastPos > MsgBase 至少有一个完整数据包
			while (lastPos >= sizeof(MsgBase))
			{
				if (lastPos >= phead->m_dataLen)
				{
					std::cout << "接收来自客户端 " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << " 的信息：" << ((MsgTalk*)phead)->GetBuff() << std::endl;
					memcpy(recvBuff, recvBuff + phead->m_dataLen, lastPos - phead->m_dataLen);
					lastPos -= phead->m_dataLen;
				}
				else
				{
					break;
				}
			}
		}

		else
		{
			std::cout << "客户端" << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << "断开连接" << std::endl;
			closesocket(clientSocket);
			break;
		}
	}
	return 0;
}

int main()
{
	// 初始化socket库
	SocketInit socketInit;

	// 监听socket
	SOCKET listenerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenerSocket == INVALID_SOCKET)
	{
		std::cout << "创建监听Socket失败...\n";
		return 1;
	}

	// 设置监听的ip和端口
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PORT);
	addrServer.sin_addr.s_addr = inet_addr(IP);

	// 绑定
	if (bind(listenerSocket, (const sockaddr*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR)
	{
		std::cout << "绑定失败...\n";
		closesocket(listenerSocket);
		return 1;
	}

	// 监听
	if (listen(listenerSocket, 20) == SOCKET_ERROR)
	{
		std::cout << "监听失败...\n";
		closesocket(listenerSocket);
		return 1;
	}
	else
	{
		std::cout << "开始监听...\n";
	}
	
	while (true)
	{
		sockaddr_in addrClient;
		int len = sizeof(addrClient);
		SOCKET clientSocket = accept(listenerSocket, (sockaddr*)&addrClient, &len);

		if (clientSocket == SOCKET_ERROR)
		{
			std::cout << "接收客户端失败！\n";
			closesocket(listenerSocket);
			return 1;
		}
		else
		{
			// 获取对应客户端ip 端口
			sockaddr_in addr;
			int len = sizeof(addr);
			getpeername(clientSocket, (sockaddr*)&addr, &len);

			std::cout << "1客户端 " << inet_ntoa(addrClient.sin_addr) << ":" << ntohs(addrClient.sin_port) << " 接入服务器..." << std::endl;
			//std::cout << "1客户端 " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << " 接入服务器..." << std::endl;
			// 新建线程处理
			CreateThread(NULL, NULL, ThreadProc, &clientSocket, NULL, NULL);
		}

	}

	closesocket(listenerSocket);

	return 0;
}