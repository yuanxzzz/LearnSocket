#include <iostream>
#include <WinSock2.h>
#include <ws2def.h>
#include <WS2tcpip.h>
#include "SocketInit.hpp"
using namespace std;

// 创建socket
void LearnSocket()
{
	// TCP
	// 使用ipv4
	// TCP协议对应的是 面向流的连接 使用TCP协议
	SOCKET tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// UDP协议面向数据报
	SOCKET udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
}

/*socket命名
显示命名：
直接将地址族（ipv4还是ipv6）、端口、ip地址与socket进行绑定（bind）

这个叫做 sockaddr结构
sockaddr
而sockaddr_in将sockaddr中的sa_data拆分为了端口和ip

隐式命名：
使用connect的时候直接与对应服务器绑定的时候，会赋予当前clientSocket一个随机端口号
*/
void LearnSockaddr()
{
	/*sockaddr与sockaddr_in的转化

	sockaddr转sockaddr_in*/
	// 创建一个通用的套接字地址结构体
	struct sockaddr addr1;
	addr1.sa_family = AF_INET;
	// 假设 sa_data 存储了一个 IPv4 地址和端口号信息

	// 将 sockaddr 转换为 sockaddr_in
	struct sockaddr_in* paddr = reinterpret_cast<struct sockaddr_in*>(&addr1);
	// 或者使用 static_cast 进行转换
	// struct sockaddr_in* paddr = static_cast<struct sockaddr_in*>(&addr);

	// 访问 sockaddr_in 中的成员
	//std::cout << "IP address: " << inet_ntoa(paddr->sin_addr) << std::endl;
	std::cout << "Port number: " << ntohs(paddr->sin_port) << std::endl;

	// 创建一个 IPv4 的套接字地址结构体
	struct sockaddr_in addr2;
	addr2.sin_family = AF_INET;
	addr2.sin_port = htons(1234);
	//addr2.sin_addr.s_addr = inet_addr("192.168.1.100");
	inet_pton(AF_INET, "192.168.1.100", &addr2.sin_addr);
	//inet_pton(AF_INET, "192.168.1.100", &addr2.sin_addr);


	// 将 sockaddr_in 转换为 sockaddr
	struct sockaddr* paddr2 = (struct sockaddr*)&addr2;

	// 使用 bind 函数绑定套接字地址
	//bind(sockfd2, paddr2, sizeof(addr2));
}

// 将socket和IP端口号进行绑定
void LearnBind()
{
	// 先随便创建一个socket 这里我们使用tcp
	SOCKET tcpSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//然后创建一个sockaddr 一般都会使用socketaddr_in分别将ip和端口号进行赋值
	sockaddr_in tcpSockaddrIn;
	tcpSockaddrIn.sin_family = AF_INET;
	tcpSockaddrIn.sin_port = htons(1234);
	inet_pton(AF_INET, "192.168.8.41", &tcpSockaddrIn.sin_addr);
	sockaddr* paddr = (sockaddr*)&tcpSockaddrIn;

	// 最后传入长度 不同协议族的长度不同
	bind(tcpSocket, paddr, sizeof(paddr));
}

// 监听
void LearnListen()
{
	// 使用TCP时 服务器需要进行监听来和客户端进行连接
	// 这里传入listen的socket需要显示具名 进行bind过

	// 先随便创建一个listenersocket
	SOCKET listenerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//然后创建一个sockaddr 一般都会使用socketaddr_in分别将ip和端口号进行赋值
	sockaddr_in tcpSockaddrIn;
	tcpSockaddrIn.sin_family = AF_INET;
	tcpSockaddrIn.sin_port = htons(1234);
	inet_pton(AF_INET, "192.168.8.41", &tcpSockaddrIn.sin_addr);
	sockaddr* paddr = (sockaddr*)&tcpSockaddrIn;

	// 最后传入长度 不同协议族的长度不同
	bind(listenerSocket, paddr, sizeof(paddr));

	// backlog 半连接 最大个数
	int ret = listen(listenerSocket, 20);
}

//连接
void LearnConnect()
{
	//首先创建一个客户端socket
	SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1235);
	inet_pton(AF_INET, "192.168.8.41", &addr.sin_addr);

	sockaddr* paddr = (sockaddr*)&addr;

	// 使用UDP不用具名直接进行绑定 connect会自动在用户定义的服务范围选择一个没有使用的端口号
	// 这里传入的是远程ip和端口
	connect(clientSocket, paddr, sizeof(paddr));
}

// 服务器在开始监听后进行接受连接
void LearnAccept()
{
	// 假设我们这里定义了监听socket
	SOCKET listenerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// 从监听socket上接受一个等待连接的请求后，accept返回一个新的socket（没有具名）
	sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	// 返回的socket就已经和ip端口绑定好了
	SOCKET clientSocket = accept(listenerSocket, (sockaddr*)&cliaddr, &clilen);
}

// 发送
void LearnSend()
{
	// 连接过后才能进行send
	// 定义一个字节数组
	char sendline[1024];

	// 服务器对客户端进行发送
	// 服务器在accept时拿到了客户端连接的socket
	SOCKET listenerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	SOCKET clientSocket = accept(listenerSocket, (sockaddr*)&cliaddr, &clilen);
	// 最后一个参数控制发送行为
	send(clientSocket, sendline, strlen(sendline), 0);

	// 客户端对服务器进行发送
	//直接把自己的socket传进去
	SOCKET clientSocket2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//然后进行send
	// 返回发送的字节数，使用TCP可能会小于传入的len（分段和从新组装）
	send(clientSocket2, sendline, strlen(sendline), 0);
}

void LearnSendto()
{
	SOCKET thisSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	char sendline[1024];
	// 目标地址
	sockaddr_in targetaddr;
	// 使用UDP进行发送
	//WSASend();
	sendto(thisSocket, sendline, strlen(sendline), MSG_WAITALL, (sockaddr*)&targetaddr, sizeof(targetaddr));
}

// 接收
void LearnRecv()
{
	char buf[1024];
	SOCKET thisSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// 只能用于已连接的socket
	// 返回接收到的字节数
	int recv_len = recv(thisSocket, buf, strlen(buf), 0);
}

void LearnRecvfrom()
{
	SOCKET thisSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sockaddr_in fromaddr;
	char buf[1024];
	// 用与没链接的socket UDP
	socklen_t len = sizeof(fromaddr);
	int recvfrom_len = recvfrom(thisSocket, buf, strlen(buf), 0, (sockaddr*)&fromaddr,&len);

}

void LearnShutdownAndClose()
{
	SOCKET socket;
	shutdown(socket, SD_BOTH);
	closesocket(socket);
}
//
//int main()
//{
//	SocketInit socket_init;
//	
//}


