#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include<string>
#include <time.h>
#include<io.h>

using namespace std;
const int Mlenx = 10000;


const unsigned char ACK = 0x01;
const unsigned char SYN = 0x02;
const unsigned char SF = 0x04;
const unsigned char EF = 0x08;
const unsigned char FIN = 0x10;



double RTO = 10000;
double EstimatedRTT = 1000;
double DevRTT = 0;


char buffer[200000000];
int len;

int curSeq = 0;


SOCKET client;
SOCKADDR_IN serverAddr, clientAddr;

string File_Path = "E:\\计算机网络\\实验3.1\\sendfiles\\";
string File_Des_Path = "E:\\计算机网络\\实验3.1\\recvfiles\\";
vector<string> files;
long handle;
struct _finddata_t info;


void addSampleRTT(double sampleRTT)
{
    EstimatedRTT = 0.875 * EstimatedRTT + 0.125 * sampleRTT;
    DevRTT = 0.75 * DevRTT + 0.25 * abs(sampleRTT - EstimatedRTT);
    RTO = EstimatedRTT + 4 * DevRTT;
}



USHORT checksum(char* buf, int len) {

    unsigned char* ubuf = (unsigned char*)buf;
    register ULONG sum = 0;
    while (len) {
        USHORT temp = USHORT(*ubuf << 8) + USHORT(*(ubuf + 1));
        sum += temp;
        if (sum & 0xFFFF0000)
        {
            sum &= 0xFFFF;
            sum++;
        }
        ubuf += 2;
        len -= 2;
    }
    return ~(sum & 0xFFFF);
}
void wave_hand() {
    int tot_fail = 0;
    while (1) {

        char tmp[6];
        tmp[2] = FIN;
        tmp[3] = curSeq;
        tmp[4] = 6 >> 8;
        tmp[5] = 6 & 0xff;
        USHORT ck = checksum(tmp + 2, 4);
        tmp[0] = ck >> 8;
        tmp[1] = ck & 0xff;
        sendto(client, tmp, 6, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

        double begintime = clock();
        char recv[6];
        int lentmp = sizeof(serverAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 6, 0, (sockaddr*)&serverAddr, &lentmp) == SOCKET_ERROR) {
            if (clock() - begintime > RTO) {
                fail_send = 1;
                tot_fail++;
                break;
            }
        }
        double sampleRTT = clock() - begintime;
        if (fail_send == 0 && checksum(recv, 6) == 0 && recv[2] == (FIN + ACK) && recv[3] == curSeq) {
            addSampleRTT(sampleRTT);
            curSeq = curSeq == 0 ? 1 : 0;
            break;
        }
        else {
            if (tot_fail == 10) {
                printf("断开失败，释放资源");
                break;
            }
            continue;
        }
    }
}
void shake_hand() {
    while (1) {
        char tmp[6];
        tmp[2] = SYN;
        tmp[3] = curSeq;
        tmp[4] = 6 >> 8;
        tmp[5] = 6 & 0xff;
        USHORT ck = checksum(tmp + 2, 4);
        tmp[0] = ck >> 8;
        tmp[1] = ck & 0xff;
        sendto(client, tmp, 6, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

        int begintime = clock();
        char recv[6];
        int lentmp = sizeof(clientAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 6, 0, (sockaddr*)&serverAddr, &lentmp) == SOCKET_ERROR)
            if (clock() - begintime > RTO) {
                fail_send = 1;
                break;
            }
        double sampleRTT = clock() - begintime;
        if (fail_send == 0 && checksum(recv, 6) == 0 && recv[2] == (SYN + ACK) && recv[3] == curSeq) {
            addSampleRTT(sampleRTT);
            curSeq = curSeq == 0 ? 1 : 0;
            return;

        }
    }
}

bool send_package(char* message, int lent, bool last) {

    char* tmp;
    int tmp_len;

    int length = lent % 2 == 0 ? lent : lent + 1;

    tmp = new char[length + 6];
    tmp[length + 5] = 0;

    if (last) {
        tmp[2] = EF;
    }
    else {
        tmp[2] = SF;
    }
    tmp[3] = curSeq;

    tmp[4] = (lent + 6) >> 8;
    tmp[5] = (lent + 6) & 0xff;
    

    for (int i = 6; i < lent + 6; i++)
        tmp[i] = message[i - 6];


    USHORT ck = checksum(tmp + 2, length + 4);

    tmp[0] = ck >> 8;
    tmp[1] = ck & 0xff;


    tmp_len = lent + 6;
    int send_to_cnt = 0;

    while (1) {
        send_to_cnt++;
        sendto(client, tmp, tmp_len, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        int begintime = clock();
        char recv[6];
        int lentmp = sizeof(serverAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 6, 0, (sockaddr*)&serverAddr, &lentmp) == SOCKET_ERROR)
            if (clock() - begintime > RTO) {
                fail_send = 1;
                if (send_to_cnt == 10)
                {
                    wave_hand();
                    return false;
                }
                break;
            }
        double sampleRTT = clock() - begintime;
        if (fail_send == 0 && checksum(recv, 6) == 0 && recv[2] == ACK && recv[3] == curSeq) {
            addSampleRTT(sampleRTT);
            curSeq = curSeq == 0 ? 1 : 0;
            return true;
        }
    }
}

void send_message(char* message, int lent) {
    int package_num = lent / Mlenx + (lent % Mlenx != 0);//看要发送几个包

    for (int i = 0; i < package_num; i++) {
        bool last = (i == (package_num - 1));
        int length = last ? lent - (package_num - 1) * Mlenx : Mlenx;
        
        send_package(message + i * Mlenx, length, last);

        if (i % 10 == 0)
            printf("此文件已经发送%.2f%%\n", (float)i / package_num * 100);
    }
}


int main() {
    memset(buffer, 0, sizeof(buffer));
    WSADATA wsadata;
    int error = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (error) {
        printf("init error");
        return 0;
    }
    string serverip;
    while (1) {
        cout << "请输入接收方ip地址:";
        cin >> serverip;

        if (inet_addr(serverip.c_str()) == INADDR_NONE) {
            cout << "IP地址不合法,请重新输入" << endl;
            continue;
        }
        break;
    }

    int port;
    cout << "请输入端口号:";
    cin >> port;

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(serverip.c_str());
    client = socket(AF_INET, SOCK_DGRAM, 0);

    int time_out = 1000;//10s超时
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(time_out));

    if (client == INVALID_SOCKET) {
        cout << "Sockrt init erroe!!!" << endl;
        return 0;
    }


    handle = _findfirst((File_Path + "*").c_str(), &info);
    if (handle == -1)
    {
        cout << "failed to open directory..." << endl;
        return -1;
    }
    do {
        files.push_back(info.name);
    } while (!_findnext(handle, &info));
    for (int i = 2; i < files.size(); i++)
        cout << "(" << i - 2 << ") " << files[i] << endl;


    int input;
    //把文件全部存储在buf中
    string filename;
    while (1) {
        cout << "输入要发送的文件序号: ";
        while (1) {
            cin >> input;
            if (input >= 0 && input < files.size() - 2)
                break;
        }
        filename = File_Path + files[input + 2];
        ifstream fin(filename.c_str(), ifstream::binary);
        if (!fin) {
            cout << "文件无法打开" << endl;
            continue;
        }
        unsigned char t = fin.get();
        while (fin) {
            buffer[len++] = t;
            t = fin.get();
        }
        fin.close();
        break;
    }

    

    cout << "开始两次握手" << endl;
    shake_hand();
    cout << "两次握手完成，准备发送文件名" << endl;
    send_message((char*)(files[input + 2].c_str()), files[input + 2].length());
    cout << "文件名发送完成，准备发送文件内容" << endl;
    send_message(buffer, len);
    cout << "文件内容发送完成，准备两次挥手" << endl;
    wave_hand();
    cout << "连接断开" << endl;
    closesocket(client);


    cout << RTO << endl;
    WSACleanup();
    return 0;
}