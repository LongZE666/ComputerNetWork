#include"server_class.h"

Server::Server():RecvBuf{0},SendBuf{0}{}
Server::~Server() {
	clean();
}
void Server::clean() { //��Դ������
	for (auto& tmpfile : sockOver) { //�ر����ӣ��ر�overlapped IO
		closesocket(tmpfile.sock);
		WSACloseEvent(tmpfile.overlap.hEvent);
	}
	//����ȫ�˿ڹر���
	if (INVALID_HANDLE_VALUE != completionPort)
		CloseHandle(completionPort);
	WSACleanup();
}
bool Server::start() {//��ʼ��������
	cout << "������" << endl;
	WSADATA wsadata;//�������
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		cout << "�����ʼ��ʧ��" << endl;
		return false;
	}
	else
		cout << "�����ʼ���ɹ�" << endl;
	//У��汾��
	if (HIBYTE(wsadata.wVersion) != 2 || LOBYTE(wsadata.wVersion) != 2) {
		cout << "�汾����" << endl;
		WSACleanup();
		return false;
	}
	//��ʼ��Ƕ����
	SOCKET ServerSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == ServerSocket) {
		cout << "Ƕ���ֳ�ʼ��ʧ��" << endl;
		WSACleanup();
		return false;
	}
	else
		cout << "Ƕ���ֳ�ʼ���ɹ�" << endl;
	// ��ʼ��IP�˿ںź�Э������Ϣ
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(10086);
	inet_pton(AF_INET, "127.0.0.1", &ServerAddr.sin_addr);
	//�󶨶˿�
	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR))) {
		cout << "��ʧ��" << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return false;
	}
	else
		cout << "�󶨳ɹ�" << endl;
	//���ú�����������ɶ˿�
	completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	if (0 == completionPort) {
		cout << "��ɶ˿ڴ���ʧ��" << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return false;
	}
	else
		cout << "��ɶ˿ڴ����ɹ�" << endl;
	//����ɶ˿�
	if (completionPort != CreateIoCompletionPort((HANDLE)ServerSocket, completionPort, 0, 0)) {
		cout << "����ȫ�˿�ʧ����" << endl;
		CloseHandle(completionPort);
		closesocket(ServerSocket);
		WSACleanup();
		return false;
	}
	else
		cout << "����ȫ�˿ڳɹ�" << endl;
	//����
	if (SOCKET_ERROR == listen(ServerSocket, SOMAXCONN)) {
		cout << "����ʧ����" << endl;
		closesocket(ServerSocket);
		CloseHandle(completionPort);
		WSACleanup();
		return false;
	}
	else
		cout << "�����ɹ�" << endl;
	/*�ѷ�����socket��ΪsockOver�б�ĵ�һ��
	* ���Ҹ���id=0
	* ��ʼ��overlap
	*/
	sockOver.emplace_back(SocketOverlapped{});
	sockOver.begin()->sock = ServerSocket;
	sockOver.begin()->id = 0;
	sockOver.begin()->overlap.hEvent = WSACreateEvent();

	sockItems[ServerSocket] = sockOver.begin();

	cout << "��������ʼ�����" << endl;
	return 1;
}
int Server::Accept() {//���տͻ�����������
	//ʹ�õ���WSASocket���������һ������ΪWSA_FLAG_OVERLAPPED
	//��Ϊʹ�õ�����ȫ�˿ڣ��첽����
	SOCKET Client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET == Client) {
		cout << "�ͻ���Ƕ���ִ���ʧ��" << endl;
		return WSAGetLastError();
	}
	//�����ŷ���sockOver�������󵽣��������ǵ�һ��
	sockOver.emplace_back(SocketOverlapped{});
	sockOver.back().sock = Client;
	sockOver.back().id = int(Client);
	sockOver.back().overlap.hEvent = WSACreateEvent();

	sockItems[Client] = --sockOver.end();
	DWORD count;
	//AceptEx�������첽ִ�У�ʹ��Ч��Ҫ��Accept�ã���������ȫ�˿�
	bool res = AcceptEx(sockOver.begin()->sock, Client, RecvBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&count, &sockItems.at(Client)->overlap);

	int error = WSAGetLastError();
	if (0 == error || ERROR_IO_PENDING == error)
		return 0;
	else {//����������ʧ�ܣ���sockOver��sockItems�д�ŵĶ�Ӧ����Ĩȥ
		closesocket(Client);
		WSACloseEvent(sockItems.at(Client)->overlap.hEvent);
		sockOver.erase(sockItems.at(Client));
		sockItems.erase(Client);
		return error;
	}
}
int Server::Receive(int sock) {//���պ���
	/*
	* �������ǰѽ��յ�����Ϣ�ŵ�RecvBuf�У�Ҳ���Ǳ�����Ϣ��������
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
int Server::Send(int sock) {//���ͺ���
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
void Server::Send_Msg() {//�������ķ�����Ϣ����
	//����ʵ����Ϣ�ĵ�����Ⱥ��
	string message;
	getline(cin, message);
	if (message.empty())
		return;
	auto position = message.find(":");
	if (position != string::npos) {
		//�����ǻ�ȡҪ������Ϣ��id�������ж��id
		//��Ϊ":��Ϣ"��ʽ�����������н������ӵĿͻ��˷���
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
		if (IDs.empty()) { //�����пͻ��˷���
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
	while (!server->is_Quit) {//�������Ƿ��˳�
		bool res = GetQueuedCompletionStatus(Port, &numberofbytes, &sock, &lpOverlapped, INFINITE);
		//�߳����Ŷӵȴ���Ϣ
		int thisSocket = static_cast<int>(sock);//������ʱ��Ŀͻ���
		if (!res) {//�������
			int error = GetLastError();
			if (64 == error) {//��һ�����пͻ��˶Ͽ�����
				//��ʱҪ������Դ�������Ͽ����ӵĿͻ�����Ϣɾȥ
				cout << "�ͻ��ˡ�" << server->sockItems.at(sock)->id << "��������" << endl;
				closesocket(server->sockItems.at(thisSocket)->sock);
				WSACloseEvent(server->sockItems.at(thisSocket)->overlap.hEvent);
				server->sockOver.erase(server->sockItems.at(thisSocket));
				server->sockItems.erase(thisSocket);
				continue;
			}
			if (WSA_WAIT_TIMEOUT == error) {
				//ʱ�䳬ʱ���ȴ��˺ܾ�Ҳû���¼�����
				continue;
			}
			cout << "�̻߳�ȡ��Ϣʧ��" << endl;
			continue;
		}
		else{//����������¼�����
			if (0 == sock) {// �Ƿ��Ƿ�����socket
				//��Ϊ�ǵ��¼�������GetQueuedCompletionStatus�Ż���Ӧ
				//��run�������ȵ���һ��Accept�����ŻὨ���߳��¼�
				//���ͻ��˰󶨵���ȫ�˿���
				auto Client = (--server->sockOver.end())->sock;
				HANDLE tmp_port = CreateIoCompletionPort((HANDLE)Client, server->completionPort, Client, 0);
				if (tmp_port != server->completionPort) {
					cout << "����ɶ˿�ʧ��" << endl;
					closesocket(Client);
					server->sockOver.erase(server->sockItems.at(Client));
					server->sockItems.erase(Client);
					continue;
				}
				//�󶨳ɹ����������Ϣ���ͽ��ս׶�
				server->sockOver.back().id = int(Client);
				sprintf_s(server->SendBuf, ":��%d����ӭ��½", server->sockItems.at(Client)->id);
				server->Send(server->sockItems.at(Client)->sock);
				server->Receive(server->sockItems.at(Client)->sock);
				server->Accept();//����Ǽ����ȴ�����
				cout << "�ͻ��ˡ�" << server->sockItems.at(Client)->id << "������" << endl;

			}
			else{
				if (numberofbytes == 0) {//�����˳�
					cout << "�ͻ��ˡ�" << server->sockItems.at(sock)->id << "������" << endl;
					//��Դ����ɾȥ��Ӧ�ͻ�����Ϣ
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
						// Ҫ���մ���Ϣ��id Ŀ��id(socket)����_tmp��ֵ
						ss >> tmp;
						if (tmp) {
								sprintf_s(server->SendBuf, "%d:%s", server->sockItems.at(thisSocket)->id, server->RecvBuf + position + 1);
								cout << server->SendBuf << endl;
								if (server->sockItems.count(tmp)) {
									server->Send(tmp);
									server->RecvBuf[0] = 0;
									cout << "��Ϣת�����ͻ��ˡ�" << server->sockItems.at(thisSocket)->id << "�����ͻ��ˡ�" << tmp << "��" << endl;
								}
								else{
									cout << "�ͻ��ˡ�" << tmp << "��������" << endl;
									sprintf_s(server->SendBuf, ":�ͻ��ˡ�%d��������", tmp);
									server->Send(server->sockItems.at(thisSocket)->id);

								}
							
						}
						else{//����ǶԷ�����˵��
							cout << "�ͻ��ˡ�" << server->sockItems.at(thisSocket)->sock << "���Է�����˵" << server->RecvBuf + position + 1 << endl;
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

Server& Server::getInstance() {//����ʵ��
	static Server server;
	return server;
}

void Server::run() {//���к���
	if (!start())//��ʼ��
		return;
	if (Accept() != 0) { //�Ƚ���һ������
		cout << "���ܴ���" << endl;
		return;
	}
	//��������Ժ����������Ŀ���߳�
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	int CPUCroeNumber = si.dwNumberOfProcessors;
	vector<HANDLE>numthread; //�߳�����
	for (int i = 0; i < CPUCroeNumber; i++) {
		numthread.push_back(CreateThread(0, 0, ThreadProcess, this, 0, nullptr));
		
	}
	while (!is_Quit){//������ҲҪ��˵��
		Send_Msg();
	}
	for (int i = 0; i < CPUCroeNumber; i++)
		CloseHandle(numthread.at(i));
}