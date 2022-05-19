#include <iostream>
#include<process.h>
#include<WinSock2.h>
#include<ctime>

using namespace std;

#pragma comment(lib,"Ws2_32.lib")

void Receive(void*);
void Send(void*);

bool is_Quit = false;


int main(){
    cout << "客户端" << endl;

    WSADATA wsadata;
    if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
        cout << "嵌套字初始化失败" << endl;
        return 0;
    }

    SOCKET client = INVALID_SOCKET;

	if ((client = socket(AF_INET, SOCK_STREAM, 0)) == SOCKET_ERROR){
		cout << "套接字创建失败!" << endl;
		return 0;
	}
	else
		cout << "嵌套字创建成功" << endl;

	struct sockaddr_in serverAddr;		
	memset(&serverAddr, 0, sizeof(sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");  
	serverAddr.sin_port = htons(10086);


	if (SOCKET_ERROR == connect(client, (SOCKADDR*)&serverAddr, sizeof(serverAddr))) {
		cout << "与服务器端连接失败" << endl;
		return 0;
	}
	else
	{
		cout << "与服务器端连接成功" << endl;
	}

	_beginthread(Receive, 0, &client);
	_beginthread(Send, 0, &client);

    while (!is_Quit) {
        Sleep(1000);
    }

	if (client != INVALID_SOCKET) {
		closesocket(client);
		client = INVALID_SOCKET;
	}

	WSACleanup();
	cout << "客户端退出！" << endl;
	return 0;
}

void Receive(void* param)
{
    while (!is_Quit) {
        SOCKET client = *(SOCKET*)(param);
        char recvbuf[MAXBYTE] = { 0 };
        char server_time[20];
        memset(server_time, 0, sizeof(server_time));
        if (SOCKET_ERROR == recv(client, recvbuf, MAXBYTE, 0))
        {
            is_Quit = true;
            cout << "数据接受失败！" << endl;
            break;
        }
        else {
            cout << "接收成功：" << endl;
            memcpy(server_time, recvbuf, 19 * sizeof(char));
            cout << endl << "服务器端发送时间：" << server_time << " 消息：";
            for (int i = 19; i < sizeof(recvbuf); i++)
                cout << recvbuf[i];
            cout << endl << endl;

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
void Send(void* param)
{
    while (!is_Quit)
    {
        time_t  t;
        char buf[20];
        memset(buf, 0, sizeof(buf));
        struct tm* tmp;
        t = time(NULL);
        tmp = localtime(&t);
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tmp);


        SOCKET revClientSocket = *(SOCKET*)(param);
        char sendbuf[MAXBYTE] = { 0 };

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