#include <iostream>
#include<process.h>
#include<WinSock2.h>
#include<ctime>

using namespace std;

#pragma comment(lib,"Ws2_32.lib")

void Receive(void* param);
void Send(void* param);

bool is_Quit = false;

int main()
{
    cout << "服务器端" << endl;

    WSADATA wsadata;
    if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
        cout << "嵌套字初始化失败" << endl;
        return 0;
    }
    SOCKET server = INVALID_SOCKET;
    SOCKET client = INVALID_SOCKET;

    if ((server = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR){
        cout << "服务器套接字创建失败！" << endl;
        return 0;
    }

    struct sockaddr_in serverAddr;		
    memset(&serverAddr, 0, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY); 
    serverAddr.sin_port = htons(10086);

    if (SOCKET_ERROR == bind(server, (SOCKADDR*)&serverAddr, sizeof(serverAddr))) {
        cout << "嵌套字绑定失败" << endl;
        return 0;
    }
    else
    {
        cout << "绑定成功" << endl;
    }

    if (SOCKET_ERROR == listen(server, SOMAXCONN)) {
        cout << "监听失败" << endl;
        return 0;
    }
    else
    {
        cout << "监听成功" << endl;
    }

    sockaddr_in addrClient = { 0 };
    int addrsize = sizeof(addrClient);
    cout << "正在等待客户端连接" << endl;
    client = accept(server, (SOCKADDR*)&addrClient, &addrsize);
    if (INVALID_SOCKET == client) {
        cout << "与客户端连接失败" << endl;
        return 0;
    }
    else
    {
        cout << "与客户端连接成功" << endl;
    }
    _beginthread(Receive, 0, &client);
    _beginthread(Send, 0, &client);

    while (!is_Quit) {
        Sleep(1000);
    }

    closesocket(server);
    closesocket(client);


    WSACleanup();
    cout << "服务器停止了" << endl;
    return 0;

}

void Receive(void* param)	 
{
    while (!is_Quit){
        SOCKET client = *(SOCKET*)(param);
        char recvbuf[MAXBYTE] = {0};	 
        char client_time[20];
        memset(client_time, 0, sizeof(client_time));
        if (SOCKET_ERROR == recv(client, recvbuf, MAXBYTE, 0))
        {
            is_Quit = true;
            cout << "数据接受失败！" << endl;
            break;
        }
        else {
            memcpy(client_time, recvbuf, 19 * sizeof(char));
            cout << endl << "客户端时间：" << client_time << " 消息内容:";
            for (int i = 19; i < sizeof(recvbuf); i++)
                cout << recvbuf[i];
            cout << endl;

            time_t  t;
            char buf[20];
            memset(buf, 0, sizeof(buf));
            struct tm* tmp;
            t = time(NULL);
            tmp = localtime(&t);
            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tmp);
            cout << "当前时间：" << buf << " 你要发送的内容是：";

            memset(recvbuf, 0, sizeof(recvbuf));
        }
    }
}
void Send(void* param)	 {
    while (!is_Quit){
        time_t  t;
        char buf[20];
        memset(buf, 0, sizeof(buf));
        struct tm* tmp;
        t = time(NULL);
        tmp = localtime(&t);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tmp);


        SOCKET revClientSocket = *(SOCKET*)(param);
        char sendbuf[MAXBYTE] = {0};

        cout << "当前时间：" << buf << " 你要发送的内容是：";
        strcpy(sendbuf, buf);
        string _tmp;
        cin >> _tmp;
        if (_tmp == "quit") {
            is_Quit = true;
            continue;
        }
        strcat(sendbuf, _tmp.c_str());
  
        if (SOCKET_ERROR == send(revClientSocket, sendbuf, strlen(sendbuf), 0)) {
            is_Quit = true;
            cout << "发送消息失败！" << endl;
            continue;
        }
        else
            cout << "发送成功" << endl << endl;
    }
}