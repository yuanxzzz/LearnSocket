#include "ChatRoomClient.h"

int main()
{
	ChatRoomClient client;
	client.Run();
	
	// 等待资源清理
	Sleep(500);
	return 0;
}