#pragma warning(disable:4996)

#include <iostream>
#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"

int main()
{
	SocketInit socketInit;

	// 创建客户端socket
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == SOCKET_ERROR)
	{
		std::cout << "客户端套接字创建失败！" << std::endl;
		return 1;
	}

	// 连接
	sockaddr_in addrListener;
	addrListener.sin_family = AF_INET;
	addrListener.sin_port = htons(PORT);
	addrListener.sin_addr.s_addr = inet_addr(IP);
	if (connect(clientSocket, (const sockaddr*)&addrListener, sizeof(addrListener)) == SOCKET_ERROR)
	{
		std::cout << "连接服务器失败！" << std::endl;
		closesocket(clientSocket);
		return 1;
	}
	else
	{
		std::cout << "连接服务器成功！" << std::endl;
		sockaddr_in addr;
		int len = sizeof(addr);
		getsockname(clientSocket, (sockaddr*)&addr, &len);
		std::cout << "本地地址 " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << std::endl;
	}

	while (true)
	{
		char sendBuff[1024] = { 0 };
		MsgTalk msgTalk;
		std::cin >> msgTalk.GetBuff();
		//std::cin >> sendBuff;

		int sendCount = send(clientSocket, (const char*)&msgTalk, msgTalk.m_dataLen, NULL);
		//int sendCount = send(clientSocket, sendBuff, msgTalk.m_dataLen, NULL);
		
		if (sendCount <= 0)
		{
			std::cout << "发送失败，断开连接！" << std::endl;
			break;
		}
	}

	closesocket(clientSocket);

	return 0;
}