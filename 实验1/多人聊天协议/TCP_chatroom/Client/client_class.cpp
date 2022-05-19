#include"client_class.h"

bool Client::start() { //��ʼ��
	cout << "�ͻ���" << endl;
	WSADATA wsadata;
	if (0 != WSAStartup(MAKEWORD(2, 2), &wsadata)) { //�������
		cout << "��ʼ��ʧ��" << endl;
		return false;
	}
	else{
		cout << "��ʼ���ɹ�" << endl;
	}
	//У��汾��
	if (HIBYTE(wsadata.wVersion) != 2 || LOBYTE(wsadata.wVersion) != 2){
		cout << "�汾�Ŵ���" << WSAGetLastError() << endl;
		return false;
	}
	//����Ƕ����
	Server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == Server) {
		cout << "Ƕ���ִ���ʧ��" << endl;
		return false;
	}
	else
		cout << "Ƕ���ִ����ɹ�" << endl;
	// ��ʼ��IP�˿ںź�Э������Ϣ
	SOCKADDR_IN ServerAddr;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(10086);
	inet_pton(AF_INET, "127.0.0.1", & ServerAddr.sin_addr);
	//���������������
	if (SOCKET_ERROR == connect(Server, (SOCKADDR*)&ServerAddr, sizeof(SOCKADDR))) {
		cout << "���ӷ�������ʧ��:" << WSAGetLastError() << endl;
		return false;
	}
	else
		cout << "���ӷ������ɹ�" << endl;

	return true;
}
void Client::clean() {//�ͻ��˹رպ�������Դ
	if (Server)
		closesocket(Server);
	WSACleanup();
}
Client::~Client() {
	clean();
}
//������Ϣ
void Client::Receive() {
	char RecvBuf[MAXBYTE] = { 0 };//������Ϣ������
	while (!is_Quit){//����δ�Ͽ�����״̬
		if (SOCKET_ERROR == recv(Server, RecvBuf, sizeof(RecvBuf), 0)) {
			//�����Ϣ����ʧ��
			is_Quit = true;
			cout << "�ͻ��˶���" << endl;
			return;
		}
		else{
			/*
			* �����Ǵ����յ�����Ϣ
			* ��Ϊ���͸�ʽ�� �ͻ���id����Ϣ
			* �����յ���Ϣʱ�ɸ���":"ǰ��id��֪����ϢԴ
			* ����0�����Ƿ�������������Ϣ
			*/
			int id = 0;
			string msg(RecvBuf);
			auto position = msg.find(":");
			istringstream temp(msg.substr(0, position));
			temp >> id;
			if (0 == id) {
				cout << "��������" << RecvBuf + position + 1 << endl;
			}
			else{
				cout << id << "�ſͻ��ˣ�" << RecvBuf + position + 1 << endl;
			}
		}
	}
}
//���ڷ�����Ϣ
void Client::Send() {
	char SendBuf[MAXBYTE] = { 0 };//������Ϣ������
	string msg;
	while (!is_Quit) {
		msg.clear();
		/*
		* ʹ��getline����Ϊ��Ϣ������пո�
		* �м��ÿո�ָ��ֹcin�����ո�ض���Ϣ
		*/
		getline(cin, msg);
		if (msg == "quit") {
			is_Quit = true;
			return;
		}


		if (msg.empty())//������Ϊ�գ�����������
			continue;
		else if (msg == "quit"){//ִ���˳�����
			is_Quit = true;
			return;
		}
		else{//���͵���Ϣ�б�����":"���ţ�����Ϊ��Ч��Ϣ
			auto position = msg.find(":");
			if (position != string::npos) {
				strcpy_s(SendBuf, msg.c_str());
				if (SOCKET_ERROR == send(Server, SendBuf, sizeof(SendBuf), 0)) {
					cout << "����ʧ����,�ط�����" << endl;
				}
				else
				{
					cout << "���ͳɹ�" << endl;
				}
			}
		}
	}
}
void Client::run() {//���к���
	if (!start())//����������
		return;
	thread r(&Client::Receive, this);//����һ���߳����ڽ�����Ϣ
	r.detach();
	Send();//���������ڷ�����Ϣ
}