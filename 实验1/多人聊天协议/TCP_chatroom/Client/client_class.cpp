#include"client_class.h"

bool Client::start() { //初始化
	cout << "客户端" << endl;
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) { //打开网络库
		cout << "初始化失败" << endl;
		return false;
	}
	else{
		cout << "初始化成功" << endl;
	}
	//校验版本号
	if (HIBYTE(wsadata.wVersion) != 2 || LOBYTE(wsadata.wVersion) != 2){
		cout << "版本号错误：" << WSAGetLastError() << endl;
		return false;
	}
	//创建嵌套字
	Server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == Server) {
		cout << "嵌套字创建失败" << endl;
		return false;
	}
	else
		cout << "嵌套字创建成功" << endl;
	// 初始化IP端口号和协议族信息
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(10086);
	inet_pton(AF_INET, "127.0.0.1", & ServerAddr.sin_addr);
	//与服务器进行连接
	if (SOCKET_ERROR == connect(Server, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR))) {
		cout << "连接服务器端失败:" << WSAGetLastError() << endl;
		return false;
	}
	else
		cout << "连接服务器成功" << endl;

	return true;
}
void Client::clean() {//客户端关闭后，清理资源
	if (Server)
		closesocket(Server);
	WSACleanup();
}
Client::~Client() {
	clean();
}
//接收消息
void Client::Receive() {
	char RecvBuf[MAXBYTE] = { 0 };//接收消息的数组
	while (!is_Quit){//若是未断开连接状态
		if (SOCKET_ERROR == recv(Server, RecvBuf, sizeof(RecvBuf), 0)) {
			//如果消息接收失败
			is_Quit = true;
			cout << "客户端断联" << endl;
			return;
		}
		else{
			/*
			* 这里是处理收到的消息
			* 因为发送格式是 客户端id：消息
			* 所以收到消息时可根据":"前的id，知道消息源
			* 若是0，则是服务器端来的消息
			*/
			int id = 0;
			string msg(RecvBuf);
			auto position = msg.find(":");
			istringstream temp(msg.substr(0, position));
			temp >> id;
			if (0 == id) {
				cout << "服务器：" << RecvBuf + position + 1 << endl;
			}
			else{
				cout << id << "号客户端：" << RecvBuf + position + 1 << endl;
			}
		}
	}
}
//用于发送消息
void Client::Send() {
	char SendBuf[MAXBYTE] = { 0 };//发送消息的数组
	string msg;
	while (!is_Quit) {
		msg.clear();
		/*
		* 使用getline是因为消息间可以有空格
		* 中间用空格分割，防止cin遇到空格截断消息
		*/
		getline(cin, msg);
		if (msg == "quit") {
			is_Quit = true;
			return;
		}


		if (msg.empty())//若输入为空，则重新输入
			continue;
		else if (msg == "quit"){//执行退出操作
			is_Quit = true;
			return;
		}
		else{//发送的消息中必须有":"符号，否则为无效消息
			auto position = msg.find(":");
			if (position != string::npos) {
				strcpy_s(SendBuf, msg.c_str());
				if (SOCKET_ERROR == send(Server, SendBuf, sizeof(SendBuf), 0)) {
					cout << "发送失败了,重发试试" << endl;
				}
				else
				{
					cout << "发送成功" << endl;
				}
			}
		}
	}
}
void Client::run() {//运行函数
	if (!start())//首先是启动
		return;
	thread r(&Client::Receive, this);//创建一个线程用于接收消息
	r.detach();
	Send();//主函数用于发送消息
}