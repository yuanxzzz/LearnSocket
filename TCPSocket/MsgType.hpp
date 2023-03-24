#pragma once

/**
 * \brief 最大缓存长度
 */
#define MAX_BUFF_LEN 1024

 /**
  * \brief 聊天室成员最大数
  */
#define MAX_MEMBER_COUNT 50

  /**
   * \brief 包最大长度
   */
#define PACKET_MAX_SIZE 1024

   /**
	* \brief 房间最大个数
	*/
#define MAX_ROOM_COUNT 50

/**
 * \brief 消息类型
 */
enum MSG_TYPE
{
	/**
	 * \brief 显示房间列表
	 */
	MSG_SHWO,

	/**
	 * \brief 加入房间
	 */
	MSG_JOIN,

	/**
	 * \brief 创建房间
	 */
	MSG_CREATE,

	/**
	 * \brief 进行聊天
	 */
	MSG_TALK,

	/**
	 * \brief 离开房间
	 */
	MSG_LEAVE,

	/**
	 * \brief 显示房间列表回复
	 */
	MSG_SHOWREPLY,

	/**
	 * \brief 广播
	 */
	MSG_BROAD,

	/**
	 * \brief 错误信息
	 */
	MSG_ERROR,

	/**
	 * \brief 连接信息
	 * 返回给客户端其用户id
	 */
	MSG_CONN
};

/**
 * \brief 错误类型
 */
enum ERROR_TYPE
{
	/**
	 * \brief 创建房间失败
	 */
	ERROR_CREATE,

	/**
	 * \brief 加入房间失败
	 */
	ERROR_JOIN,

	/**
	 * \brief 未进入房间
	 */
	ERROR_NO_ENTER,

	/**
	 * \brief 已在房间内
	 */
	ERROR_ALREADY_ENTER
};

/**
 * \brief 消息类
 */
class MsgBase
{
public:
	MSG_TYPE m_msgType; // 消息类型
	int m_dataLen; //消息长度
};

#pragma region S2C

/**
 * \brief 请求展示房间信息
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
 * \brief 请求加入房间
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
 * \brief 离开房间
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
 * \brief 请求创建房间
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
 * \brief 聊天消息
 */
class MsgTalk :public MsgBase
{
	/**
	 * \brief 发送消息的用户id
	 */
	int m_id;

	/**
	 * \brief 消息字符数组
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
 * \brief 请求房间信息回复
 */
class MsgShowReply :public MsgBase
{
	/**
	 * \brief 房间信息
	 */
	struct RoomInfo
	{
		/**
		 * \brief 房间最大容量
		 */
		int m_maxMemberCount = MAX_MEMBER_COUNT;

		/**
		 * \brief 房间当前人数
		 */
		int m_currentMemberCount;

		/**
		 * \brief 房间ID
		 */
		int m_roomID;
	};

	/**
	 * \brief 房间信息数组
	 */
	RoomInfo m_roomInfo[MAX_ROOM_COUNT];

public:
	MsgShowReply()
	{
		m_msgType = MSG_SHOWREPLY;
		m_dataLen = sizeof(MsgShowReply);
	}

	/**
	 * \brief 获取每个房间信息
	 * \return
	 */
	RoomInfo* GetRoomInfo()
	{
		return m_roomInfo;
	}

	/**
	 * \brief 房间数量
	 */
	int m_roomCount;
};

/**
 * \brief 广播消息
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
 * \brief 聊天消息
 */
class MsgError :public MsgBase
{
	ERROR_TYPE m_errorCode;
public:
	MsgError(ERROR_TYPE e)
	{
		m_msgType = MSG_ERROR;
		m_dataLen = sizeof(MsgTalk);
		// 设置错误码
		m_errorCode = e;
	}

	int getErrorCode()
	{
		return m_errorCode;
	}
};

/**
 * \brief 连接成功后发送至客户端
 * 告知用户对应的id
 */
class MsgConn :public MsgBase
{
	/**
	 * \brief 用户的id
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