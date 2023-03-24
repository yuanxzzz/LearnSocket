#pragma once
#include <iostream>
#include <map>
#include <vector>
#include <string>

#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"

class ChatRoomServer;
using std::cout;
using std::endl;



/**
 * \brief 自定义OverLapped结构体
 */
struct MyOverLapped :public OVERLAPPED
{
	// 构造
	MyOverLapped(SOCKET sock, int ID)
	{
		memset(this, 0, sizeof(MyOverLapped));
		m_socket = sock;
		m_WSAbufRecv.buf = m_buffRecv;
		m_WSAbufRecv.len = sizeof(m_buffRecv);
		/*m_WSAbufSend.buf = m_buffSend;
		m_WSAbufSend.len = sizeof(m_buffSend);*/
		m_ID = ID;
		m_lastPos = 0;
		m_roomId = 0;
	}

	/**
	 * \brief 对应的socket
	 */
	SOCKET m_socket;

	/**
	 * \brief 接收缓存
	 */
	char m_buffRecv[MAX_BUFF_LEN * 2];

	/**
	 * \brief 接收缓存
	 */
	WSABUF m_WSAbufRecv;

	///**
	// * \brief 发送缓存
	// */
	//char m_buffSend[MAX_BUFF_LEN];

	///**
	// * \brief 发送缓存
	// */
	//WSABUF m_WSAbufSend;

	// 对应ID
	int m_ID;

	// 上一次解析到的位置
	int m_lastPos;

	/**
	 * \brief 所在的房间id
	 */
	int m_roomId;
};

/**
 * \brief 聊天室
 */
class ChatRoom
{
private:
	/**
	 * \brief 聊天室最大成员数
	 */
	int m_maxMemberNum;
	
	/**
	 * \brief 聊天室成员
	 */
	std::vector<int> m_memberVec;

public:
	/**
	 * \brief 聊天室房间号
	 */
	int m_ID;

	/**
	 * \brief 当前人数
	 */
	int m_currentCount;

	/**
	 * \brief 临界资源
	 */
	CRITICAL_SECTION m_secRoom;
public:
	/**
	 * \brief 构造函数
	 */
	ChatRoom(int id);

	/**
	 * \brief 析构
	 */
	~ChatRoom();

	/**
	 * \brief 进入聊天室
	 * \param id 客户端id
	 * \return 
	 */
	int Enter(ChatRoomServer* server, int id);

	/**
	 * \brief 离开聊天室
	 * \param id 客户端id
	 * \return 
	 */
	int Leave(ChatRoomServer* server, int id);

	/**
	 * \brief 获取该聊天室中所有客户端id
	 * \return 
	 */
	std::vector<int> GetMembers();

	/**
	 * \brief 对房间内的用户进行广播
	 * \param server 
	 * \param msgBase 
	 */
	void Broadcast(ChatRoomServer* server, MsgBase* msgBase);
};

/**
 * \brief 聊天室服务器
 */
class ChatRoomServer
{
private:
	/**
	 * \brief 初始化socket库
	 */
	SocketInit m_socketInit;

	/**
	 * \brief 保存的结构体数量
	 */
	int m_index;

	/**
	 * \brief ID与对应结构体的映射
	 */
	std::map<int, MyOverLapped*> m_mapID2OverLapped;

	/**
	 * \brief 持有的完成端口句柄
	 */
	HANDLE m_hCompletionPort;

	/**
	 * \brief 线程列表句柄
	 */
	HANDLE* m_pThread;

	/**
	 * \brief 线程数量
	 */
	int m_threadCount;

	/**
	 * \brief ID与对应聊天室映射
	 */
	std::map<int, ChatRoom*> m_mapID2Room;

	/**
	 * \brief 房间个数
	 */
	int m_roomCount;

	/**
	 * \brief 房间序号
	 */
	int m_indexRoom;

	/**
	 * \brief 临界资源
	 */
	CRITICAL_SECTION m_sec;
public:
	/**
	 * \brief 构造
	 */
	ChatRoomServer();

	/**
	 * \brief 析构
	 */
	~ChatRoomServer();

	/**
	 * \brief 清理资源
	 */
	void Clear();

	/**
	 * \brief 启动
	 * \return 是否正常启动
	 */
	int StartUp();

	/**
	 * \brief 接收连接
	 * \return 是否成功
	 */
	int PostAccept();

	/**
	 * \brief 接收数据
	 * \param id 结构体对应id
	 * \return 是否成功
	 */
	int PostRecv(int id);

	/**
	 * \brief 发送数据
	 * \param id 结构体对应id
	 * \return 是否成功
	 */
	int PostSend(int id, MsgBase* msgBase);

	/**
	 * \brief 处理消息
	 * \param id 结构体对应id
	 * \param msgBase 消息
	 */
	void DealWithMsg(int id, MsgBase* msgBase);

	/**
	 * \brief 线程函数
	 * \param lp 
	 * \return 
	 */
	static DWORD WINAPI ThreadProc(LPVOID lp);
};

