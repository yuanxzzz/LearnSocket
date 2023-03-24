#ifndef _SOCKET_INIT_H_
#define _SOCKET_INIT_H_

#include <cstdio>
#include<WinSock2.h>

#pragma comment(lib,"ws2_32.lib")

/**
 * \brief Ĭ��IP
 */
#define IP "127.0.0.1"
/**
 * \brief Ĭ�϶˿ں�
 */
#define PORT 12345

class SocketInit
{
public:
	SocketInit()
	{
		WORD socketVersion = MAKEWORD(2, 2);
		WSADATA wasData;
		if(WSAStartup(socketVersion, &wasData) != 0)
		{
			printf("��̬���ӿ����ʧ�ܣ�\n");
		}
	}
	~SocketInit()
	{
		WSACleanup();
	}
};

#endif

