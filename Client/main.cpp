#include <WINSOCK2.H>
#include <STDIO.H>
#include <iostream>
#include <cstring>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

bool thread_terminate = false;
void message_recv(SOCKET sock);
void message_send(SOCKET sock);

int main()
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA data;
	if (WSAStartup(sockVersion, &data) != 0)
	{
		return 0;
	}
	SOCKET sclient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sclient == INVALID_SOCKET)
	{
		printf("invalid socket!");
		return 0;
	}

	printf("输入serverIP：");

	char buffer[255];
	memset(buffer, 0, sizeof(buffer));
	fgets(buffer, 255, stdin);

	int len = strlen(buffer);
	printf("len=%d\n", len);

	while (true)
	{
		sockaddr_in serAddr;
		serAddr.sin_family = AF_INET;
		serAddr.sin_port = htons(8888);
		serAddr.sin_addr.S_un.S_addr = inet_addr(buffer);
		if (connect(sclient, (sockaddr*)&serAddr, sizeof(serAddr)) == SOCKET_ERROR)
		{
			//连接失败 
			printf("connect error !\n");
			closesocket(sclient);
			continue;
		}

		//连接成功 
		printf("connected\n");
		break;
	}

	// 启动接收线程
	std::thread t_recv(message_recv, sclient);

	// 等待输入
	memset(buffer, 0, sizeof(buffer));
	while (fgets(buffer, sizeof(buffer)-1, stdin)) /* break with ^D or ^Z */
	{
		// 将输入的消息发给server
		send(sclient, buffer, strlen(buffer), 0);

		// 重置内存
		memset(buffer, 0, sizeof(buffer));
	}

	thread_terminate = true;

	closesocket(sclient);

	WSACleanup();
	return 0;

}

void message_recv(SOCKET sock)
{
	while (!thread_terminate)
	{
		//接收数据  
		char revData[255];
		int ret = recv(sock, revData, sizeof(revData) - 1, 0);
		if (ret > 0)
		{
			revData[ret] = 0;
			printf("%s\n", revData);
		}
	}

	closesocket(sock);
}

