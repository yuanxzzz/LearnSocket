#ifndef _SOCKET_INIT_H_
#define _SOCKET_INIT_H_

#include <cstdio>
#include<WinSock2.h>

#pragma comment(lib,"ws2_32.lib")

/**
 * \brief 默认IP
 */
#define IP "127.0.0.1"
/**
 * \brief 默认端口号
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
			printf("动态链接库加载失败！\n");
		}
	}
	~SocketInit()
	{
		WSACleanup();
	}
};

#endif

