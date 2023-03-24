#pragma once

#include <iostream>
#include <iomanip>

#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"

using std::cin;
using std::cout;
using std::endl;

/**
 * \brief 最大缓存长度
 */
#define MAX_BUFF_LEN 1024

class ChatRoomClient
{
	/**
	 * \brief socket库初始化
	 */
	SocketInit m_socketInit;

	/**
	 * \brief 对应的socket
	 */
	SOCKET m_socket;

	/**
	 * \brief 接收缓存
	 */
	char m_buffRecv[MAX_BUFF_LEN * 2];

	/**
	 * \brief 发送缓存
	 */
	char m_buffSend[MAX_BUFF_LEN];

	/**
	 * \brief 线程句柄
	 */
	HANDLE m_hThread;

	/**
	 * \brief 是否正在运行
	 */
	bool m_isRunnning;

	/**
	 * \brief 上一次解析到的位置
	 */
	int m_lastPos;

	/**
	 * \brief 在服务器对应的id
	 */
	int m_id;

public:
	/**
	 * \brief 构造
	 */
	ChatRoomClient();

	/**
	 * \brief 析构
	 */
	~ChatRoomClient();

	/**
	 * \brief 清理 
	 */
	void Clear();

	/**
	 * \brief 运行
	 */
	int Run();

	/**
	 * \brief 接收
	 * \return 
	 */
	int PostRecv();

	/**
	 * \brief 发送
	 * \return 
	 */
	int PostSend();

	/**
	 * \brief 发送消息
	 * \param msgBase 
	 * \return 
	 */
	int SendMsg(MsgBase* msgBase);

	/**
	 * \brief 处理消息
	 * \param msgBase 消息
	 */
	void DealWithMsg(MsgBase* msgBase);

	/**
	 * \brief 线程函数
	 * \param lp 
	 * \return 
	 */
	static DWORD WINAPI ThreadProc(LPVOID lp);
};

