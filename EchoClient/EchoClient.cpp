#pragma warning(disable:4996)

#include <iostream>
#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"

int main()
{
	SocketInit socketInit;

	// �����ͻ���socket
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == SOCKET_ERROR)
	{
		std::cout << "�ͻ����׽��ִ���ʧ�ܣ�" << std::endl;
		return 1;
	}

	// ����
	sockaddr_in addrListener;
	addrListener.sin_family = AF_INET;
	addrListener.sin_port = htons(PORT);
	addrListener.sin_addr.s_addr = inet_addr(IP);
	if (connect(clientSocket, (const sockaddr*)&addrListener, sizeof(addrListener)) == SOCKET_ERROR)
	{
		std::cout << "���ӷ�����ʧ�ܣ�" << std::endl;
		closesocket(clientSocket);
		return 1;
	}
	else
	{
		std::cout << "���ӷ������ɹ���" << std::endl;
		sockaddr_in addr;
		int len = sizeof(addr);
		getsockname(clientSocket, (sockaddr*)&addr, &len);
		std::cout << "���ص�ַ " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << std::endl;
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
			std::cout << "����ʧ�ܣ��Ͽ����ӣ�" << std::endl;
			break;
		}
	}

	closesocket(clientSocket);

	return 0;
}