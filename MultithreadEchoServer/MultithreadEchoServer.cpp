#pragma warning(disable:4996)

#include <iostream>
#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"

/**
 * \brief ����socket���Ӽ���������
 * \param lp 
 * \return 
 */
DWORD WINAPI ThreadProc(LPVOID lp)
{
	SOCKET clientSocket = *(SOCKET*)lp;

	// ��ȡ��Ӧ�ͻ���ip �˿�
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
			// ѭ�����
			// lastPos > MsgBase ������һ���������ݰ�
			while (lastPos >= sizeof(MsgBase))
			{
				if (lastPos >= phead->m_dataLen)
				{
					std::cout << "�������Կͻ��� " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << " ����Ϣ��" << ((MsgTalk*)phead)->GetBuff() << std::endl;
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
			std::cout << "�ͻ���" << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << "�Ͽ�����" << std::endl;
			closesocket(clientSocket);
			break;
		}
	}
	return 0;
}

int main()
{
	// ��ʼ��socket��
	SocketInit socketInit;

	// ����socket
	SOCKET listenerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenerSocket == INVALID_SOCKET)
	{
		std::cout << "��������Socketʧ��...\n";
		return 1;
	}

	// ���ü�����ip�Ͷ˿�
	sockaddr_in addrServer;
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(PORT);
	addrServer.sin_addr.s_addr = inet_addr(IP);

	// ��
	if (bind(listenerSocket, (const sockaddr*)&addrServer, sizeof(addrServer)) == SOCKET_ERROR)
	{
		std::cout << "��ʧ��...\n";
		closesocket(listenerSocket);
		return 1;
	}

	// ����
	if (listen(listenerSocket, 20) == SOCKET_ERROR)
	{
		std::cout << "����ʧ��...\n";
		closesocket(listenerSocket);
		return 1;
	}
	else
	{
		std::cout << "��ʼ����...\n";
	}
	
	while (true)
	{
		sockaddr_in addrClient;
		int len = sizeof(addrClient);
		SOCKET clientSocket = accept(listenerSocket, (sockaddr*)&addrClient, &len);

		if (clientSocket == SOCKET_ERROR)
		{
			std::cout << "���տͻ���ʧ�ܣ�\n";
			closesocket(listenerSocket);
			return 1;
		}
		else
		{
			// ��ȡ��Ӧ�ͻ���ip �˿�
			sockaddr_in addr;
			int len = sizeof(addr);
			getpeername(clientSocket, (sockaddr*)&addr, &len);

			std::cout << "1�ͻ��� " << inet_ntoa(addrClient.sin_addr) << ":" << ntohs(addrClient.sin_port) << " ���������..." << std::endl;
			//std::cout << "1�ͻ��� " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port) << " ���������..." << std::endl;
			// �½��̴߳���
			CreateThread(NULL, NULL, ThreadProc, &clientSocket, NULL, NULL);
		}

	}

	closesocket(listenerSocket);

	return 0;
}