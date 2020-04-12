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

	printf("����serverIP��");

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
			//����ʧ�� 
			printf("connect error !\n");
			closesocket(sclient);
			continue;
		}

		//���ӳɹ� 
		printf("connected\n");
		break;
	}

	// ���������߳�
	std::thread t_recv(message_recv, sclient);

	// �ȴ�����
	memset(buffer, 0, sizeof(buffer));
	while (fgets(buffer, sizeof(buffer)-1, stdin)) /* break with ^D or ^Z */
	{
		// ���������Ϣ����server
		send(sclient, buffer, strlen(buffer), 0);

		// �����ڴ�
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
		//��������  
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

