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
 * \brief �Զ���OverLapped�ṹ��
 */
struct MyOverLapped :public OVERLAPPED
{
	// ����
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
	 * \brief ��Ӧ��socket
	 */
	SOCKET m_socket;

	/**
	 * \brief ���ջ���
	 */
	char m_buffRecv[MAX_BUFF_LEN * 2];

	/**
	 * \brief ���ջ���
	 */
	WSABUF m_WSAbufRecv;

	///**
	// * \brief ���ͻ���
	// */
	//char m_buffSend[MAX_BUFF_LEN];

	///**
	// * \brief ���ͻ���
	// */
	//WSABUF m_WSAbufSend;

	// ��ӦID
	int m_ID;

	// ��һ�ν�������λ��
	int m_lastPos;

	/**
	 * \brief ���ڵķ���id
	 */
	int m_roomId;
};

/**
 * \brief ������
 */
class ChatRoom
{
private:
	/**
	 * \brief ����������Ա��
	 */
	int m_maxMemberNum;
	
	/**
	 * \brief �����ҳ�Ա
	 */
	std::vector<int> m_memberVec;

public:
	/**
	 * \brief �����ҷ����
	 */
	int m_ID;

	/**
	 * \brief ��ǰ����
	 */
	int m_currentCount;

	/**
	 * \brief �ٽ���Դ
	 */
	CRITICAL_SECTION m_secRoom;
public:
	/**
	 * \brief ���캯��
	 */
	ChatRoom(int id);

	/**
	 * \brief ����
	 */
	~ChatRoom();

	/**
	 * \brief ����������
	 * \param id �ͻ���id
	 * \return 
	 */
	int Enter(ChatRoomServer* server, int id);

	/**
	 * \brief �뿪������
	 * \param id �ͻ���id
	 * \return 
	 */
	int Leave(ChatRoomServer* server, int id);

	/**
	 * \brief ��ȡ�������������пͻ���id
	 * \return 
	 */
	std::vector<int> GetMembers();

	/**
	 * \brief �Է����ڵ��û����й㲥
	 * \param server 
	 * \param msgBase 
	 */
	void Broadcast(ChatRoomServer* server, MsgBase* msgBase);
};

/**
 * \brief �����ҷ�����
 */
class ChatRoomServer
{
private:
	/**
	 * \brief ��ʼ��socket��
	 */
	SocketInit m_socketInit;

	/**
	 * \brief ����Ľṹ������
	 */
	int m_index;

	/**
	 * \brief ID���Ӧ�ṹ���ӳ��
	 */
	std::map<int, MyOverLapped*> m_mapID2OverLapped;

	/**
	 * \brief ���е���ɶ˿ھ��
	 */
	HANDLE m_hCompletionPort;

	/**
	 * \brief �߳��б���
	 */
	HANDLE* m_pThread;

	/**
	 * \brief �߳�����
	 */
	int m_threadCount;

	/**
	 * \brief ID���Ӧ������ӳ��
	 */
	std::map<int, ChatRoom*> m_mapID2Room;

	/**
	 * \brief �������
	 */
	int m_roomCount;

	/**
	 * \brief �������
	 */
	int m_indexRoom;

	/**
	 * \brief �ٽ���Դ
	 */
	CRITICAL_SECTION m_sec;
public:
	/**
	 * \brief ����
	 */
	ChatRoomServer();

	/**
	 * \brief ����
	 */
	~ChatRoomServer();

	/**
	 * \brief ������Դ
	 */
	void Clear();

	/**
	 * \brief ����
	 * \return �Ƿ���������
	 */
	int StartUp();

	/**
	 * \brief ��������
	 * \return �Ƿ�ɹ�
	 */
	int PostAccept();

	/**
	 * \brief ��������
	 * \param id �ṹ���Ӧid
	 * \return �Ƿ�ɹ�
	 */
	int PostRecv(int id);

	/**
	 * \brief ��������
	 * \param id �ṹ���Ӧid
	 * \return �Ƿ�ɹ�
	 */
	int PostSend(int id, MsgBase* msgBase);

	/**
	 * \brief ������Ϣ
	 * \param id �ṹ���Ӧid
	 * \param msgBase ��Ϣ
	 */
	void DealWithMsg(int id, MsgBase* msgBase);

	/**
	 * \brief �̺߳���
	 * \param lp 
	 * \return 
	 */
	static DWORD WINAPI ThreadProc(LPVOID lp);
};

