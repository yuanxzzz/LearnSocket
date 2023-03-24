#pragma once

#include <iostream>
#include <iomanip>

#include "../TCPSocket/Socketinit.hpp"
#include "../TCPSocket/MsgType.hpp"

using std::cin;
using std::cout;
using std::endl;

/**
 * \brief ��󻺴泤��
 */
#define MAX_BUFF_LEN 1024

class ChatRoomClient
{
	/**
	 * \brief socket���ʼ��
	 */
	SocketInit m_socketInit;

	/**
	 * \brief ��Ӧ��socket
	 */
	SOCKET m_socket;

	/**
	 * \brief ���ջ���
	 */
	char m_buffRecv[MAX_BUFF_LEN * 2];

	/**
	 * \brief ���ͻ���
	 */
	char m_buffSend[MAX_BUFF_LEN];

	/**
	 * \brief �߳̾��
	 */
	HANDLE m_hThread;

	/**
	 * \brief �Ƿ���������
	 */
	bool m_isRunnning;

	/**
	 * \brief ��һ�ν�������λ��
	 */
	int m_lastPos;

	/**
	 * \brief �ڷ�������Ӧ��id
	 */
	int m_id;

public:
	/**
	 * \brief ����
	 */
	ChatRoomClient();

	/**
	 * \brief ����
	 */
	~ChatRoomClient();

	/**
	 * \brief ���� 
	 */
	void Clear();

	/**
	 * \brief ����
	 */
	int Run();

	/**
	 * \brief ����
	 * \return 
	 */
	int PostRecv();

	/**
	 * \brief ����
	 * \return 
	 */
	int PostSend();

	/**
	 * \brief ������Ϣ
	 * \param msgBase 
	 * \return 
	 */
	int SendMsg(MsgBase* msgBase);

	/**
	 * \brief ������Ϣ
	 * \param msgBase ��Ϣ
	 */
	void DealWithMsg(MsgBase* msgBase);

	/**
	 * \brief �̺߳���
	 * \param lp 
	 * \return 
	 */
	static DWORD WINAPI ThreadProc(LPVOID lp);
};

