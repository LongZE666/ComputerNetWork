#pragma once
#include<iostream>
#include<string>
#include<WinSock2.h>
#include<MSWSock.h>
#include<mswsock.h>
#include<thread>
#include<sstream>
#include<WS2tcpip.h>
#include<list>
#include<vector>
#include<unordered_map>
using namespace std;
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")
struct SocketOverlapped{ //保存客户端嵌套字，客户端id，及重叠io
	SOCKET sock;
	int id = 0;
	OVERLAPPED overlap;
};
class Server {
private:
	bool is_Quit = false; //服务器是否退出
	HANDLE completionPort = INVALID_HANDLE_VALUE; //完成端口
	list<SocketOverlapped> sockOver; //存放所有的SocketOverlapped信息
	unordered_map<int, decltype(sockOver.begin())> sockItems; //主要是方便根据id找socket，不用遍历
	char RecvBuf[MAXBYTE]; //接收数组
	char SendBuf[MAXBYTE]; //发送数组
	bool start(); //启动函数
	void clean(); //资源清理函数
	int Accept(); //接收来自客户端的连接
	int Receive(int); //接收客户端消息
	int Send(int); //将客户端消息转发出去
	void Send_Msg(); //服务器也有向客户端说话的功能
	friend DWORD WINAPI ThreadProcess(LPVOID);//回调函数
public:
	Server();
	Server(const Server&) = delete;
	Server(Server&&) = default;
	~Server();
	static Server& getInstance();//可以获得实例
	void run(); //启动函数
};