#pragma once

/**
 * \brief ��󻺴泤��
 */
#define MAX_BUFF_LEN 1024

 /**
  * \brief �����ҳ�Ա�����
  */
#define MAX_MEMBER_COUNT 50

  /**
   * \brief ����󳤶�
   */
#define PACKET_MAX_SIZE 1024

   /**
	* \brief ����������
	*/
#define MAX_ROOM_COUNT 50

/**
 * \brief ��Ϣ����
 */
enum MSG_TYPE
{
	/**
	 * \brief ��ʾ�����б�
	 */
	MSG_SHWO,

	/**
	 * \brief ���뷿��
	 */
	MSG_JOIN,

	/**
	 * \brief ��������
	 */
	MSG_CREATE,

	/**
	 * \brief ��������
	 */
	MSG_TALK,

	/**
	 * \brief �뿪����
	 */
	MSG_LEAVE,

	/**
	 * \brief ��ʾ�����б�ظ�
	 */
	MSG_SHOWREPLY,

	/**
	 * \brief �㲥
	 */
	MSG_BROAD,

	/**
	 * \brief ������Ϣ
	 */
	MSG_ERROR,

	/**
	 * \brief ������Ϣ
	 * ���ظ��ͻ������û�id
	 */
	MSG_CONN
};

/**
 * \brief ��������
 */
enum ERROR_TYPE
{
	/**
	 * \brief ��������ʧ��
	 */
	ERROR_CREATE,

	/**
	 * \brief ���뷿��ʧ��
	 */
	ERROR_JOIN,

	/**
	 * \brief δ���뷿��
	 */
	ERROR_NO_ENTER,

	/**
	 * \brief ���ڷ�����
	 */
	ERROR_ALREADY_ENTER
};

/**
 * \brief ��Ϣ��
 */
class MsgBase
{
public:
	MSG_TYPE m_msgType; // ��Ϣ����
	int m_dataLen; //��Ϣ����
};

#pragma region S2C

/**
 * \brief ����չʾ������Ϣ
 */
class MsgShow :public MsgBase
{
public:
	MsgShow()
	{
		m_msgType = MSG_SHWO;
		m_dataLen = sizeof(MsgShow);
	}
};

/**
 * \brief ������뷿��
 */
class MsgJoin :public MsgBase
{
public:
	int m_roomID;
	MsgJoin(int r)
	{
		m_msgType = MSG_JOIN;
		m_dataLen = sizeof(MsgJoin);
		m_roomID = r;
	}
};

/**
 * \brief �뿪����
 */
class MsgLeave :public MsgBase
{
public:
	MsgLeave()
	{
		m_msgType = MSG_LEAVE;
		m_dataLen = sizeof(MsgLeave);
	}
};

/**
 * \brief ���󴴽�����
 */
class MsgCreate :public MsgBase
{
public:
	MsgCreate()
	{
		m_msgType = MSG_CREATE;
		m_dataLen = sizeof(MsgCreate);
	}
};

/**
 * \brief ������Ϣ
 */
class MsgTalk :public MsgBase
{
	/**
	 * \brief ������Ϣ���û�id
	 */
	int m_id;

	/**
	 * \brief ��Ϣ�ַ�����
	 */
	char m_buff[1000];
public:
	MsgTalk()
	{
		m_msgType = MSG_TALK;
		m_dataLen = sizeof(MsgTalk);
	}

	char* GetBuff()
	{
		return m_buff;
	}

	void SetId(int id)
	{
		m_id = id;
	}

	int GetId()
	{
		return m_id;
	}
};
#pragma endregion S2C

#pragma region C2C

/**
 * \brief ���󷿼���Ϣ�ظ�
 */
class MsgShowReply :public MsgBase
{
	/**
	 * \brief ������Ϣ
	 */
	struct RoomInfo
	{
		/**
		 * \brief �����������
		 */
		int m_maxMemberCount = MAX_MEMBER_COUNT;

		/**
		 * \brief ���䵱ǰ����
		 */
		int m_currentMemberCount;

		/**
		 * \brief ����ID
		 */
		int m_roomID;
	};

	/**
	 * \brief ������Ϣ����
	 */
	RoomInfo m_roomInfo[MAX_ROOM_COUNT];

public:
	MsgShowReply()
	{
		m_msgType = MSG_SHOWREPLY;
		m_dataLen = sizeof(MsgShowReply);
	}

	/**
	 * \brief ��ȡÿ��������Ϣ
	 * \return
	 */
	RoomInfo* GetRoomInfo()
	{
		return m_roomInfo;
	}

	/**
	 * \brief ��������
	 */
	int m_roomCount;
};

/**
 * \brief �㲥��Ϣ
 */
class MsgBroad :public MsgBase
{
	char m_buff[1000];
public:
	MsgBroad()
	{
		m_msgType = MSG_BROAD;
		m_dataLen = sizeof(MsgTalk);
	}

	char* getBuff()
	{
		return m_buff;
	}
};

/**
 * \brief ������Ϣ
 */
class MsgError :public MsgBase
{
	ERROR_TYPE m_errorCode;
public:
	MsgError(ERROR_TYPE e)
	{
		m_msgType = MSG_ERROR;
		m_dataLen = sizeof(MsgTalk);
		// ���ô�����
		m_errorCode = e;
	}

	int getErrorCode()
	{
		return m_errorCode;
	}
};

/**
 * \brief ���ӳɹ��������ͻ���
 * ��֪�û���Ӧ��id
 */
class MsgConn :public MsgBase
{
	/**
	 * \brief �û���id
	 */
	int m_id;
public:
	MsgConn(int id)
	{
		m_msgType = MSG_CONN;
		m_dataLen = sizeof(MsgTalk);
		m_id = id;
	}

	int getId()
	{
		return m_id;
	}
};

#pragma endregion C2S