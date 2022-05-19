#pragma once

#include<iostream>
#include<WinSock2.h>
#include<thread>
#include<cstdlib>
#include<sstream>
#include<string>
#include<WS2tcpip.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib")


class Client {
private:
	bool is_Quit = false; //客户端是否退出
	SOCKET Server = INVALID_SOCKET; //用于和服务器端建立连接

	bool start(); //初始化工作，建立连接
	void Receive(); //接收消息
	void Send();//发送消息
	void clean(); //断线后清理资源

public:
	Client() = default; //构造函数
	~Client(); //析构函数

	void run(); //运行接口，由main函数调用


};