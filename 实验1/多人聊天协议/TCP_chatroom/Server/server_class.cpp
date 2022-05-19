#include"server_class.h"

Server::Server():RecvBuf{0},SendBuf{0}{}
Server::~Server() {
	clean();
}
void Server::clean() { //资源清理函数
	for (auto& tmpfile : sockOver) { //关闭连接，关闭overlapped IO
		closesocket(tmpfile.sock);
		WSACloseEvent(tmpfile.overlap.hEvent);
	}
	//把完全端口关闭了
	if (INVALID_HANDLE_VALUE != completionPort)
		CloseHandle(completionPort);
	WSACleanup();
}
bool Server::start() {//初始化服务器
	cout << "服务器" << endl;
	WSADATA wsadata;//打开网络库
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		cout << "网络初始化失败" << endl;
		return false;
	}
	else
		cout << "网络初始化成功" << endl;
	//校验版本号
	if (HIBYTE(wsadata.wVersion) != 2 || LOBYTE(wsadata.wVersion) != 2) {
		cout << "版本错误" << endl;
		WSACleanup();
		return false;
	}
	//初始化嵌套字
	SOCKET ServerSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == ServerSocket) {
		cout << "嵌套字初始化失败" << endl;
		WSACleanup();
		return false;
	}
	else
		cout << "嵌套字初始化成功" << endl;
	// 初始化IP端口号和协议族信息
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(10086);
	inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr);
	//绑定端口
	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR))) {
		cout << "绑定失败" << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return false;
	}
	else
		cout << "绑定成功" << endl;
	//调用函数来创建完成端口
	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (0 == completionPort) {
		cout << "完成端口创建失败" << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return false;
	}
	else
		cout << "完成端口创建成功" << endl;
	//绑定完成端口
	if (completionPort != CreateIoCompletionPort((HANDLE)ServerSocket, completionPort, 0, 0)) {
		cout << "绑定完全端口失败了" << endl;
		CloseHandle(completionPort);
		closesocket(ServerSocket);
		WSACleanup();
		return false;
	}
	else
		cout << "绑定完全端口成功" << endl;
	//监听
	if (SOCKET_ERROR == listen(ServerSocket, SOMAXCONN)) {
		cout << "监听失败了" << endl;
		closesocket(ServerSocket);
		CloseHandle(completionPort);
		WSACleanup();
		return false;
	}
	else
		cout << "监听成功" << endl;
	/*把服务器socket作为sockOver列表的第一个
	* 并且赋予id=0
	* 初始化overlap
	*/
	sockOver.emplace_back(SocketOverlapped{});
	sockOver.begin()->sock = ServerSocket;
	sockOver.begin()->id = 0;
	sockOver.begin()->overlap.hEvent = WSACreateEvent();

	sockItems[ServerSocket] = sockOver.begin();

	cout << "服务器初始化完成" << endl;
	return 1;
}
int Server::Accept() {//接收客户端连接请求
	//使用的是WSASocket函数，最后一个参数为WSA_FLAG_OVERLAPPED
	//因为使用的是完全端口，异步操作
	SOCKET Client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == Client) {
		cout << "客户端嵌套字创建失败" << endl;
		return WSAGetLastError();
	}
	//紧接着放入sockOver，先来后到，服务器是第一个
	sockOver.emplace_back(SocketOverlapped{});
	sockOver.back().sock = Client;
	sockOver.back().id = int(Client);
	sockOver.back().overlap.hEvent = WSACreateEvent();

	sockItems[Client] = --sockOver.end();
	DWORD count;
	//AceptEx函数，异步执行，使用效果要比Accept好，尤其是完全端口
	bool res = AcceptEx(sockOver.begin()->sock, Client, RecvBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&count, &sockItems.at(Client)->overlap);

	int error = WSAGetLastError();
	if (0 == error || ERROR_IO_PENDING == error)
		return 0;
	else {//若建立连接失败，则将sockOver和sockItems中存放的对应数据抹去
		closesocket(Client);
		WSACloseEvent(sockItems.at(Client)->overlap.hEvent);
		sockOver.erase(sockItems.at(Client));
		sockItems.erase(Client);
		return error;
	}
}
int Server::Receive(int sock) {//接收函数
	/*
	* 操作就是把接收到的信息放到RecvBuf中，也就是保存信息的数组中
	*/
	WSABUF wsabuf;
	wsabuf.buf = RecvBuf;
	wsabuf.len = sizeof(RecvBuf);
	DWORD count = 0;
	DWORD flag = 0;
	bool res = WSARecv(sockItems.at(sock)->sock, &wsabuf, 1, &count, &flag, &sockItems.at(sock)->overlap, nullptr);
	int error = WSAGetLastError();
	if (0 == error || WSA_IO_PENDING==error) {
		return 0;
	}
	else
		return error;
}
int Server::Send(int sock) {//发送函数
	WSABUF wsabuf;
	wsabuf.buf = SendBuf;
	wsabuf.len = sizeof(SendBuf);
	DWORD count = 0;
	DWORD flag = 0;
	bool res = WSASend(sockItems.at(sock)->sock, &wsabuf, 1, &count, flag, &sockItems.at(sock)->overlap, nullptr);
	int error = WSAGetLastError();
	if (0 == error || WSA_IO_PENDING == error) {
		return 0;
	}
	else
		return error;
}
void Server::Send_Msg() {//服务器的发送消息函数
	//可以实现消息的单发和群发
	string message;
	getline(cin, message);
	if (message.empty())
		return;
	auto position = message.find(":");
	if (position != string::npos) {
		//这里是获取要发送消息的id，可以有多个id
		//若为":消息"格式，则是向所有建立连接的客户端发送
		vector<int>IDs; 
		istringstream I(message.substr(0, position));
		int tmp;
		while (I >> tmp) {
			IDs.push_back(tmp);
		}
		size_t i = 1;
		SendBuf[0] = ':';
		for (size_t j = position + 1; j < message.size(); ++i, ++j) {
			SendBuf[i] = message.at(j);
		}
		SendBuf[i] = 0;
		if (IDs.empty()) { //向所有客户端发送
			for (const auto& tmp : sockOver) {
				Send(tmp.sock);
			}
		}
		else{
			for (auto tmp : IDs) {
				Send(tmp);
			}
		}
		return;
	}
	if (message == "quit") {
		is_Quit = true;
		Sleep(1000);
		return;     
	}
}

DWORD WINAPI ThreadProcess(LPVOID lptr) {
	Server* server = static_cast<Server*>(lptr);
	HANDLE Port = (HANDLE)server->completionPort;
	DWORD numberofbytes = 0;
	LPOVERLAPPED lpOverlapped;
	ULONG_PTR sock = 0;
	while (!server->is_Quit) {//服务器是否退出
		bool res = GetQueuedCompletionStatus(Port, &numberofbytes, &sock, &lpOverlapped, INFINITE);
		//线程们排队等待消息
		int thisSocket = static_cast<int>(sock);//保存来时间的客户端
		if (!res) {//出错情况
			int error = GetLastError();
			if (64 == error) {//第一种是有客户端断开连接
				//这时要进行资源清理，将断开连接的客户端消息删去
				cout << "客户端【" << server->sockItems.at(sock)->id << "】下线了" << endl;
				closesocket(server->sockItems.at(thisSocket)->sock);
				WSACloseEvent(server->sockItems.at(thisSocket)->overlap.hEvent);
				server->sockOver.erase(server->sockItems.at(thisSocket));
				server->sockItems.erase(thisSocket);
				continue;
			}
			if (WSA_WAIT_TIMEOUT == error) {
				//时间超时，等待了很久也没有事件发生
				continue;
			}
			cout << "线程获取消息失败" << endl;
			continue;
		}
		else{//这里就是有事件发生
			if (0 == sock) {// 是否是服务器socket
				//因为是当事件发生后GetQueuedCompletionStatus才会响应
				//而run函数会先调用一个Accept函数才会建立线程事件
				//将客户端绑定到完全端口上
				auto Client = (--server->sockOver.end())->sock;
				HANDLE tmp_port = CreateIoCompletionPort((HANDLE)Client, server->completionPort, Client, 0);
				if (tmp_port != server->completionPort) {
					cout << "绑定完成端口失败" << endl;
					closesocket(Client);
					server->sockOver.erase(server->sockItems.at(Client));
					server->sockItems.erase(Client);
					continue;
				}
				//绑定成功后，则进入消息发送接收阶段
				server->sockOver.back().id = int(Client);
				sprintf_s(server->SendBuf, ":【%d】欢迎登陆", server->sockItems.at(Client)->id);
				server->Send(server->sockItems.at(Client)->sock);
				server->Receive(server->sockItems.at(Client)->sock);
				server->Accept();//这段是继续等待连接
				cout << "客户端【" << server->sockItems.at(Client)->id << "】来辣" << endl;

			}
			else{
				if (numberofbytes == 0) {//正常退出
					cout << "客户端【" << server->sockItems.at(sock)->id << "】下线" << endl;
					//资源清理，删去对应客户端信息
					closesocket(server->sockItems.at(thisSocket)->sock);
					WSACloseEvent(server->sockItems.at(thisSocket)->overlap.hEvent);
					server->sockOver.erase(server->sockItems.at(thisSocket));
					server->sockItems.at(thisSocket);
				}
				else{
					if (server->RecvBuf[0]) {
						string str(server->RecvBuf);
						cout << str << endl;
						auto position = str.find(':');
						istringstream ss(str.substr(0, position));
						int tmp = 0;
						// 要接收此消息的id 目的id(socket)，是_tmp的值
						ss >> tmp;
						if (tmp) {
								sprintf_s(server->SendBuf, "%d:%s", server->sockItems.at(thisSocket)->id, server->RecvBuf + position + 1);
								cout << server->SendBuf << endl;
								if (server->sockItems.count(tmp)) {
									server->Send(tmp);
									server->RecvBuf[0] = 0;
									cout << "消息转发：客户端【" << server->sockItems.at(thisSocket)->id << "】到客户端【" << tmp << "】" << endl;
								}
								else{
									cout << "客户端【" << tmp << "】不存在" << endl;
									sprintf_s(server->SendBuf, ":客户端【%d】不存在", tmp);
									server->Send(server->sockItems.at(thisSocket)->id);

								}
							
						}
						else{//如果是对服务器说话
							cout << "客户端【" << server->sockItems.at(thisSocket)->sock << "】对服务器说" << server->RecvBuf + position + 1 << endl;
						}
						server->RecvBuf[0] = 0;
						server->Receive(server->sockItems.at(thisSocket)->sock);
					}
				}
			}
			
		}
	}
	return 0;
}

Server& Server::getInstance() {//返回实例
	static Server server;
	return server;
}

void Server::run() {//运行函数
	if (!start())//初始化
		return;
	if (Accept() != 0) { //先建立一个连接
		cout << "接受错误" << endl;
		return;
	}
	//开辟与电脑核心数相等数目的线程
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int CPUCroeNumber = si.dwNumberOfProcessors;
	vector<HANDLE>numthread; //线程数组
	for (int i = 0; i < CPUCroeNumber; i++) {
		numthread.push_back(CreateThread(0, 0, ThreadProcess, this, 0, nullptr));
		
	}
	while (!is_Quit){//主函数也要会说话
		Send_Msg();
	}
	for (int i = 0; i < CPUCroeNumber; i++)
		CloseHandle(numthread.at(i));
}