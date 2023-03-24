#pragma warning(disable:4996)

#include <iostream>
#include <WS2tcpip.h>

#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"


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

	// �׽��ּ���
	fd_set fdSocket;
	FD_ZERO(&fdSocket);
	FD_SET(listenerSocket, &fdSocket);

	sockaddr_in addrClient;
	int len = sizeof(addrClient);
	char recvBuff[1024] = { 0 };
	int lastPos = 0;

	while (true)
	{
		fd_set fdRead = fdSocket;
		int nRet = select(0, &fdRead, NULL, NULL, NULL);
		if(nRet > 0)
		{
			for (int i = 0; i < nRet; i++)
			{
				// �������ӽ���
				if(fdRead.fd_array[i] == listenerSocket)
				{
					SOCKET clientSocket = accept(listenerSocket, (sockaddr*)&addrClient, &len);
					if (clientSocket == SOCKET_ERROR)
					{
						std::cout << "���տͻ���ʧ�ܣ�\n";
						closesocket(listenerSocket);
						return 1;
					}
					else
					{
						std::cout << "�ͻ��� " << inet_ntoa(addrClient.sin_addr) << ":" << ntohs(addrClient.sin_port) << " ���������..." << std::endl;
						// ���뼯��
						FD_SET(clientSocket, &fdSocket);
					}
				}
				// ����Ϣ����
				else
				{
					// ��ȡ��Ӧ�ͻ���ip �˿�
					sockaddr_in addr;
					int len = sizeof(addr);
					getpeername(fdRead.fd_array[i], (sockaddr*)&addr, &len);

					int recvCount = recv(fdRead.fd_array[i], recvBuff + lastPos, PACKET_MAX_SIZE - lastPos, NULL);
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
								
								std::cout << "�������Կͻ��� " << inet_ntoa(addr.sin_addr) << ":" << ntohs(addrClient.sin_port) << " ����Ϣ��" << ((MsgTalk*)phead)->GetBuff() << std::endl;
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
						std::cout << "�ͻ���" << inet_ntoa(addr.sin_addr) << ":" << ntohs(addrClient.sin_port) << "�Ͽ�����" << std::endl;
						closesocket(fdRead.fd_array[i]);
						FD_CLR(fdRead.fd_array[i], &fdSocket);
						break;
					}
				}
			}
		}
	}

	closesocket(listenerSocket);

	return 0;
}