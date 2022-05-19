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
struct SocketOverlapped{ //����ͻ���Ƕ���֣��ͻ���id�����ص�io
	SOCKET sock;
	int id = 0;
	OVERLAPPED overlap;
};
class Server {
private:
	bool is_Quit = false; //�������Ƿ��˳�
	HANDLE completionPort = INVALID_HANDLE_VALUE; //��ɶ˿�
	list<SocketOverlapped> sockOver; //������е�SocketOverlapped��Ϣ
	unordered_map<int, decltype(sockOver.begin())> sockItems; //��Ҫ�Ƿ������id��socket�����ñ���
	char RecvBuf[MAXBYTE]; //��������
	char SendBuf[MAXBYTE]; //��������
	bool start(); //��������
	void clean(); //��Դ������
	int Accept(); //�������Կͻ��˵�����
	int Receive(int); //���տͻ�����Ϣ
	int Send(int); //���ͻ�����Ϣת����ȥ
	void Send_Msg(); //������Ҳ����ͻ���˵���Ĺ���
	friend DWORD WINAPI ThreadProcess(LPVOID);//�ص�����
public:
	Server();
	Server(const Server&) = delete;
	Server(Server&&) = default;
	~Server();
	static Server& getInstance();//���Ի��ʵ��
	void run(); //��������
};