#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <map>
#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib,"ws2_32.lib") 

struct ClientInfo
{
    SOCKET sock;
    std::thread* t;
    std::queue<char*> message;
};


// 监听套接字
SOCKET slisten = 0;

bool thread_terminate = false;
std::mutex msgMutex;
std::map<SOCKET, ClientInfo> ClientSockets;

void client_listen();
void message_send();
void message_recv(SOCKET sock);

int main(int argc, char* argv[])
{
    //初始化WSA  
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0)
    {
        return 0;
    }

    // 启动监听线程
    std::thread listen(client_listen);

    // 启动转发线程
    std::thread forward(message_send);

    // 等待输入
    char buffer[255];
    while (fgets(buffer, 255, stdin)) /* break with ^D or ^Z */
    {
        printf("%s\n", buffer);
    }

    thread_terminate = true;

    closesocket(slisten);


    WSACleanup();
    return 0;
}

void client_listen()
{
    //创建套接字  
    slisten = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (slisten == INVALID_SOCKET)
    {
        printf("socket error !");
        return;
    }

    //绑定IP和端口  
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(8888);
    sin.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(slisten, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
    {
        printf("bind error !");
    }

    //开始监听  
    if (listen(slisten, 5) == SOCKET_ERROR)
    {
        printf("listen error !");
        return;
    }

    //循环接收数据  
    SOCKET sClient;
    sockaddr_in remoteAddr;
    int nAddrlen = sizeof(remoteAddr);
    char revData[255];
    while (!thread_terminate)
    {
        printf("等待连接...\n");
        sClient = accept(slisten, (SOCKADDR*)&remoteAddr, &nAddrlen);
        if (sClient == INVALID_SOCKET)
        {
            printf("accept error !");
            continue;
        }
        printf("接受到一个连接：%s \r\n", inet_ntoa(remoteAddr.sin_addr));

        // 客户端信息
        ClientInfo info;
        info.sock = sClient;
        ClientSockets[sClient] = info;
        
        // 启动一个客户端线程
        info.t = new std::thread(message_recv, sClient);

        // 保存到队列
    }

    closesocket(slisten);
}

void message_recv(SOCKET sock)
{
    while (!thread_terminate)
    {
        auto& client = ClientSockets[sock];

        //接收数据  
        char revData[255];
        int ret = recv(sock, revData, sizeof(revData) - 1, 0);
        if (ret > 0)
        {
            revData[ret] = 0;
            printf("%s\n", revData);

            // 将消息拷贝一份
            char* msg = new char[ret+1];
            sprintf(msg, "%s", revData);
            msg[ret] = 0;

            // 将接收到的消息放入队列
            std::lock_guard<std::mutex> l(msgMutex);
            client.message.push(msg);
        }
    }

    closesocket(sock);
}

void message_send()
{
    while (!thread_terminate)
    {
        std::lock_guard<std::mutex> l(msgMutex);
        for (auto& client : ClientSockets)
        {
            auto& msg_list = client.second.message;
            while (msg_list.size() > 0)
            {
                //发送数据
                char* msg = msg_list.front();


                for (auto& c : ClientSockets)
                {
                    if (c.first != client.first)
                    {
                        send(c.first, msg, strlen(msg), 0);
                    }
                }

                msg_list.pop();

                delete msg;
            }
        }
    }
}