#pragma warning(disable:4996)

#include <iostream>
#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"

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

	sockaddr_in addrClient;
	int len = sizeof(addrClient);
	char recvBuff[1024] = { 0 };
	int lastPos = 0;
	while (true)
	{
		// 记录监听的客户端socket
		SOCKET clientSocket = accept(listenerSocket, (sockaddr*)&addrClient, &len);
		if (clientSocket == SOCKET_ERROR)
		{
			std::cout << "接收客户端失败！\n";
			closesocket(listenerSocket);
			return 1;
		}else
		{
			std::cout << "客户端 " << inet_ntoa(addrClient.sin_addr) << ":" << ntohs(addrClient.sin_port) << " 接入服务器..." << std::endl;
		}

		while (true)
		{
			// 接收客户端socket
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
						std::cout << "接收来自客户端 " << inet_ntoa(addrClient.sin_addr) << ":" << ntohs(addrClient.sin_port) << " 的信息：" << ((MsgTalk*)phead)->GetBuff() << std::endl;
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
				std::cout << "客户端断开连接" << std::endl;
				break;
			}
		}
		closesocket(clientSocket);
	}

	closesocket(listenerSocket);

	return 0;
}