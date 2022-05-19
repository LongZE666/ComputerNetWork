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
	bool is_Quit = false; //�ͻ����Ƿ��˳�
	SOCKET Server = INVALID_SOCKET; //���ںͷ������˽�������

	bool start(); //��ʼ����������������
	void Receive(); //������Ϣ
	void Send();//������Ϣ
	void clean(); //���ߺ�������Դ

public:
	Client() = default; //���캯��
	~Client(); //��������

	void run(); //���нӿڣ���main��������


};