#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include<time.h>
#include<io.h>

using namespace std;
const int Mlenx = 10000;

const unsigned char ACK = 0x01;
const unsigned char SYN = 0x02;
const unsigned char SF = 0x04;
const unsigned char EF = 0x08;
const unsigned char FIN = 0x10;



const int TIMEOUT = 5000;//毫秒
char buffer[200000000];
int len;

int curSeq = 0;


SOCKADDR_IN serverAddr, clientAddr;
SOCKET server; //选择udp协议


string File_Path = "E:\\计算机网络\\实验3.1\\sendfiles\\";
string File_Des_Path = "E:\\计算机网络\\实验3.1\\recvfiles\\";
vector<string> files;
long handle;



void output(char c) {
    cout << "ACK:" << (bool)(c & ACK) << " SYN:" << (bool)(c & SYN) << " SF:" << (bool)(c & SF) << " EF:" << (bool)(c & EF) << " FIN:" << (bool)(c & FIN);
}
/*
unsigned char checksum(char* arr, int lent) {
    unsigned char ret = arr[0];

    for (int i = 1; i < lent; i++) {
        unsigned short tmp = ret + (unsigned char)arr[i];
        tmp = tmp / (1 << 8) + tmp % (1 << 8);
        //tmp = tmp / (1 << 8) + tmp % (1 << 8);
        ret = tmp;
    }
    return ~ret;
}
*/
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
void wait_shake_hand() {
    while (1) {
        char recv[Mlenx + 8], send[8];
        memset(recv, 0, sizeof(recv));

        int connect = 0;
        int lentmp = sizeof(clientAddr);
        while (recvfrom(server, recv, Mlenx + 8, 0, (sockaddr*)&clientAddr, &lentmp) == SOCKET_ERROR);

        SHORT seq = (USHORT)((unsigned char)recv[6] << 8) + (USHORT)(unsigned char)recv[7];

        if (checksum(recv, Mlenx + 8) == 0) {

            if (recv[2] == SYN && seq != curSeq) {
                cout << "ACK丢失" << endl;

                send[2] = (SYN + ACK);
                send[3] = 0;
                send[4] = 6 >> 8;
                send[5] = 6 & 0xff;

                send[6] = seq << 8;
                send[7] = seq & 0xff;

                USHORT ck = checksum(send + 2, 6);
                send[0] = ck >> 8;
                send[1] = ck & 0xff;
                sendto(server, send, 8, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));

                cout << "收到:";
                output(recv[2]);
                cout << " Length:" << (USHORT)((unsigned char)recv[4] << 8) + (USHORT)(unsigned char)recv[5];
                cout << " Checksum:" << (USHORT)((unsigned char)recv[0] << 8) + (USHORT)(unsigned char)recv[1];
                cout << " Seq" << seq;
                cout << endl;
                cout << "发送:";
                output(send[2]);
                cout << " Length:" << (USHORT)((unsigned char)send[4] << 8) + (USHORT)(unsigned char)send[5];
                cout << " Checksum:" << ck;
                cout << " Seq:" << seq;
                cout << endl;
                continue;
            }
            else if (seq != curSeq && (recv[2] == SF || recv[2] == EF)) {

            }
        }


        if (checksum(recv, Mlenx + 8) != 0 || recv[2] != SYN || seq != curSeq)
            continue;

        //收到了SYN,若校验成功，回复
        send[2] = (SYN + ACK);
        send[3] = 0;
        send[4] = 6 >> 8;
        send[5] = 6 & 0xff;
        send[6] = curSeq << 8;
        send[7] = curSeq & 0xff;


        USHORT ck = checksum(send + 2, 6);
        send[0] = ck >> 8;
        send[1] = ck & 0xff;
        sendto(server, send, 8, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));

        cout << "收到:";
        output(recv[2]);
        cout << " Length:" << (USHORT)((unsigned char)recv[4] << 8) + (USHORT)(unsigned char)recv[5];
        cout << " Checksum:" << (USHORT)((unsigned char)recv[0] << 8) + (USHORT)(unsigned char)recv[1];
        cout << " Seq" << seq;
        cout << endl;
        cout << "发送:";
        output(send[2]);
        cout << " Length:" << (USHORT)((unsigned char)send[4] << 8) + (USHORT)(unsigned char)send[5];
        cout << " Checksum:" << ck;
        cout << " Seq:" << curSeq;
        cout << endl;

        curSeq++;

        break;
    }
}

void wait_wave_hand() {
    while (1) {
        char recv[8], send[8];
        int lentmp = sizeof(clientAddr);
        while (recvfrom(server, recv, 8, 0, (sockaddr*)&clientAddr, &lentmp) == SOCKET_ERROR);

        SHORT seq = (USHORT)((unsigned char)recv[6] << 8) + (USHORT)(unsigned char)recv[7];
        if (checksum(recv, 8) != 0 || recv[2] != (char)FIN || seq != curSeq)
            continue;

        send[2] = (ACK + FIN);
        send[3] = 0;
        send[4] = 6 >> 8;
        send[5] = 6 & 0xff;
        send[6] = curSeq >> 8;
        send[7] = curSeq & 0xff;

        USHORT ck = checksum(send + 2, 6);
        send[0] = ck >> 8;
        send[1] = ck & 0xff;
        sendto(server, send, 8, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
        cout << "收到:";
        output(recv[2]);
        cout << " Length:" << (USHORT)((unsigned char)recv[4] << 8) + (USHORT)(unsigned char)recv[5];
        cout << " Checksum:" << (USHORT)((unsigned char)recv[0] << 8) + (USHORT)(unsigned char)recv[1];
        cout << " Seq" << seq;
        cout << endl;
        cout << "发送:";
        output(send[2]);
        cout << " Length:" << (USHORT)((unsigned char)send[4] << 8) + (USHORT)(unsigned char)send[5];
        cout << " Checksum:" << ck;
        cout << " Seq:" << curSeq;
        cout << endl;
        curSeq++;
        break;
    }
}

HANDLE _send, __send;

DWORD WINAPI Send(LPVOID lparam) {
    char send[8];
    send[2] = ACK;
    send[3] = 0;
    send[4] = 8 >> 8;
    send[5] = 8 & 0xff;
    int seq = (int)lparam;
    send[6] = seq >> 8;
    send[7] = seq & 0xff;
    USHORT ck = checksum(send + 2, 6);
    send[0] = ck >> 8;
    send[1] = ck & 0xff;

    sendto(server, send, 8, 0, (sockaddr*)&clientAddr, sizeof(clientAddr));
    cout << "发送:";
    output(send[2]);
    cout << " Length:" << 8;
    cout << " Checksum:" << ck;
    cout << " Seq:" << seq;
    cout << endl;
}

 

void recv_message(char* message, int& len_recv) {
    
    int lentmp = sizeof(clientAddr);
    len_recv = 0;
    
    SHORT seq, CK;

    char recv[Mlenx + 8];
    while (1) {
        while (1) {
            memset(recv, 0, sizeof(recv));
            while (recvfrom(server, recv, Mlenx + 8, 0, (sockaddr*)&clientAddr, &lentmp) == SOCKET_ERROR);

            seq = (USHORT)((unsigned char)recv[6] << 8) + (USHORT)(unsigned char)recv[7];
            CK = checksum(recv, Mlenx + 8);
            if (CK == 0 && seq == curSeq) {
                // 正常收到的情况下啊，返回ACK
                _send = CreateThread(NULL, NULL,Send, (LPVOID)curSeq, NULL, NULL);
                

                cout << "收到:";
                output(recv[2]);
                cout << " Length:" << (USHORT)((unsigned char)recv[4] << 8) + (USHORT)(unsigned char)recv[5];
                cout << " Checksum:" << (USHORT)((unsigned char)recv[0] << 8) + (USHORT)(unsigned char)recv[1];
                cout << " Seq:" << seq;
                cout << endl;
                
                curSeq++;
                break;
            }
            else if (CK == 0 && seq != curSeq) {
                //校验通过了，序号对不上
                //这个是接收方ACK丢失，发送方重传的情况
                //亦或者是，有一条丢了，后续继续发，反正我们只要我们需要的那一条
                // 回复的序号是上一条ACK回复
                __send = CreateThread(NULL, NULL, Send, (LPVOID)(curSeq-1), NULL, NULL);
                

                cout << "(发送端丢包)收到:";
                output(recv[2]);
                cout << " Length:" << (USHORT)((unsigned char)recv[4] << 8) + (USHORT)(unsigned char)recv[5];
                cout << " Checksum:" << (USHORT)((unsigned char)recv[0] << 8) + (USHORT)(unsigned char)recv[1];
                cout << " Seq:" << seq;
                cout << endl;

            }
            //cout << checksum(recv, Mlenx + 8) << endl;

        }
        USHORT length = (USHORT)((unsigned char)recv[4] << 8) + (USHORT)(unsigned char)recv[5];
        

        
        for (int i = 8; i < length; i++) {
            message[len_recv] = recv[i];
            len_recv++;
        }
        
        if (EF == recv[2]) {
            cout << curSeq << endl;
            break;
        }
    }


}

int main() {
    WSADATA wsadata;
    int nError = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (nError) {
        printf("start error\n");
        return 0;
    }
    int Port;
    cout << "请输入服务器端口:";
    cin >> Port;

    serverAddr.sin_family = AF_INET; //使用ipv4
    serverAddr.sin_port = htons(Port); //端口
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    server = socket(AF_INET, SOCK_DGRAM, 0);

    int time_out = 1000;//1ms超时
    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(time_out));

    if (server == INVALID_SOCKET) {
        cout << "Socket init error!!!" << endl;
        closesocket(server);
        return 0;
    }
    int retVAL = bind(server, (sockaddr*)(&serverAddr), sizeof(serverAddr));
    if (retVAL == SOCKET_ERROR) {
        cout << "绑定失败" << endl;
        closesocket(server);
        WSACleanup();
        return 0;
    }
    int length = 0;


    clock_t s1, s2, e1, e2;

    cout << "等待发送端连接" << endl;
    wait_shake_hand();
    cout << "两次握手完成，准备接收数据" << endl;



    s1 = clock();
    recv_message(buffer, len);
    e1 = clock();

    cout << "文件名接收完成" << endl;
    buffer[len] = 0;
    cout << "文件名：" << buffer << endl;
    cout << "文件名长度：" << len << " bytes" << endl;


    length += len;


    string file_name(buffer);
    string Filename = File_Des_Path + file_name;

    s2 = clock();
    recv_message(buffer, len);
    e2 = clock();

    cout << "文件数据长度：" << len << " bytes" << endl;

    length = len;

    cout << "文件数据接收完成" << endl;


    wait_wave_hand();
    cout << "已断开连接" << endl;

    ofstream fout(Filename.c_str(), ofstream::binary);
    for (int i = 0; i < len; i++)
        fout << buffer[i];
    fout.close();

    double alltime = double(e2 - s2) / CLOCKS_PER_SEC;

    cout << alltime << endl;
    cout << "吞吐率:" << (double)((length * 8 / 1000) / alltime) << "Kbps" << endl;

    return 0;
}
/*
   数据报结构
   |-----16-----|---8---|---8---|
   |   checksum |  flag |  Seq  |
   |   Length   |      data     |
   |    data    |      data     |
   |   .........................|

*/