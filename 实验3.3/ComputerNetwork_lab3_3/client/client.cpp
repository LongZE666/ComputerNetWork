#pragma comment(lib, "Ws2_32.lib")

#include <iostream>
#include <winsock2.h>
#include <stdio.h>
#include <fstream>
#include <vector>
#include<string>
#include <time.h>
#include<io.h>
#include<queue>

using namespace std;
const int Mlenx = 10000;    // 发送的最多数据数


const unsigned char ACK = 0x01;
const unsigned char SYN = 0x02;
const unsigned char SF = 0x04;
const unsigned char EF = 0x08;
const unsigned char FIN = 0x10;


char buffer[200000000];    //读取的文件数据    
int len;                   //文件数据的长度
vector<char*>messages;      //存放文件数据包
queue<double> timesaver; //保存发送时间的，顺序存入
                         // 发一个数据包，存一个
                         // 如果发生超时，清空队列，重新根据sendbase发送数据

int lastSeq = 0;        // 发送文件最后一个数据报的序号
int startSeq = 0;       // 发送文件第一个数据报的序号
int curSeq = 0;         // 已经发送的最后一个数据报的编号
int SendBase = 0;       // 等待确认的第一个数据报的编号
                        // 其中 curSeq - SendBase <= windows_size

bool closeall = false;  // 用于关闭接收数据报的线程


SOCKET client;
SOCKADDR_IN serverAddr, clientAddr;

//文件
string File_Path = "E:\\计算机网络\\实验3.1\\sendfiles\\";
string File_Des_Path = "E:\\计算机网络\\实验3.1\\recvfiles\\";
vector<string> files;
long _handle;
struct _finddata_t info;


// 线程所需
HANDLE recver;


// 计时线程所需

double timeout = 2000;
double EstimatedRTT = 2000;
double DevRTT = 0;

clock_t  startTime, endTime;          // 局部计时
clock_t fileStartTime, fileEndTime;  // 一个文件的总发送时间
int  timecount = 0;
bool PreTickTime = false;             // 计时器退出标志


int state = 0;        //  0:慢启动、1:拥塞避免、2:快速恢复。在初始情况下是慢启动阶段。
int dupACKcount = 0;  // 发生了重复ACK的次数。
double cwnd = 1;      // 窗口大小,初始为1
int ssthresh = 8;     // 阈值大小
bool* ACKs;           // 数组存储。来判定重复ACK情况的，顺便解决以下累计确认的问题。


void addSampleRTT(double sampleRTT)
{
    EstimatedRTT = 0.875 * EstimatedRTT + 0.125 * sampleRTT;
    DevRTT = 0.75 * DevRTT + 0.25 * abs(sampleRTT - EstimatedRTT);
    timeout = EstimatedRTT + 4 * DevRTT;
}

USHORT checksum(char* buf, int len) {//计算校验和
    unsigned char* ubuf = (unsigned char*)buf;
    register ULONG sum = 0;
    while (len) {
        USHORT temp = USHORT(*ubuf << 8) + USHORT(*(ubuf + 1));
        sum += temp;
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF;
            sum++;
        }
        ubuf += 2;
        len -= 2;
    }
    return ~(sum & 0xFFFF);
}

// 造文件数据报的函数
char* makeDatagram(char* message, int lent, bool last, int seq) {//length是数据的长度
    char* tmp;
    int length = lent % 2 == 0 ? lent : lent + 1;

    tmp = new char[length + 8];
    tmp[length + 7] = 0;

    if (last) {
        tmp[2] = EF;
        lastSeq = seq;
    }
    else {
        tmp[2] = SF;
    }
    tmp[3] = 0;

    tmp[4] = (lent + 8) >> 8;
    tmp[5] = (lent + 8) & 0xff;
    tmp[6] = seq >> 8;
    tmp[7] = seq & 0xff;
    for (int i = 8; i < lent + 8; i++)
        tmp[i] = message[i - 8];


    USHORT ck = checksum(tmp + 2, length + 6);

    tmp[0] = ck >> 8;
    tmp[1] = ck & 0xff;


    return tmp;
}



// 两次握手和两次挥手，就不用滑动窗口了，就一个数据报。。。
void wave_hand() {
    int tot_fail = 0;
    while (1) {

        char tmp[8];
        tmp[2] = FIN;
        tmp[3] = 0;
        tmp[4] = 8 >> 8;
        tmp[5] = 8 & 0xff;
        tmp[6] = curSeq >> 8;
        tmp[7] = curSeq & 0xff;

        USHORT ck = checksum(tmp + 2, 6);
        tmp[0] = ck >> 8;
        tmp[1] = ck & 0xff;
        sendto(client, tmp, 8, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

        double begintime = clock();
        char recv[8];
        int lentmp = sizeof(serverAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 8, 0, (sockaddr*)&serverAddr, &lentmp) == SOCKET_ERROR) {
            if (clock() - begintime > timeout) {
                fail_send = 1;
                tot_fail++;
                break;
            }
        }
        double sampleRTT = clock() - begintime;
        SHORT seq = (USHORT)((unsigned char)recv[6] << 8) + (USHORT)(unsigned char)recv[7];
        if (fail_send == 0 && checksum(recv, 8) == 0 && recv[2] == (FIN + ACK) && seq == curSeq) {
            addSampleRTT(sampleRTT);
            curSeq++;
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
        char tmp[8];
        tmp[2] = SYN;
        tmp[3] = 0;
        tmp[4] = 6 >> 8;
        tmp[5] = 6 & 0xff;
        tmp[6] = curSeq << 8;
        tmp[7] = curSeq & 0xff;

        USHORT ck = checksum(tmp + 2, 6);
        tmp[0] = ck >> 8;
        tmp[1] = ck & 0xff;

        //cout << checksum(tmp, 6) << endl;

        sendto(client, tmp, 8, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));

        clock_t begintime = clock();
        char recv[8];
        int lentmp = sizeof(clientAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 8, 0, (sockaddr*)&serverAddr, &lentmp) == SOCKET_ERROR)
            if (clock() - begintime > timeout) {
                fail_send = 1;
                break;
            }
        double sampleRTT = clock() - begintime;
        SHORT seq = (USHORT)((unsigned char)recv[6] << 8) + (USHORT)(unsigned char)recv[7];
        if (fail_send == 0 && checksum(recv, 8) == 0 && recv[2] == (SYN + ACK) && seq == curSeq) {
            addSampleRTT(sampleRTT);
            curSeq++;
            return;

        }
    }
}

bool send_package(char* message, int lent, bool last) {
    char* tmp;
    int tmp_len;

    int length = lent % 2 == 0 ? lent : lent + 1;

    tmp = new char[length + 8];
    tmp[length + 7] = 0;

    if (last) {
        tmp[2] = EF;
    }
    else {
        tmp[2] = SF;
    }
    tmp[3] = 0;

    tmp[4] = (lent + 8) >> 8;
    tmp[5] = (lent + 8) & 0xff;
    tmp[6] = curSeq << 8;
    tmp[7] = curSeq & 0xff;

    for (int i = 8; i < lent + 8; i++)
        tmp[i] = message[i - 8];



    USHORT ck = checksum(tmp + 2, length + 6);

    tmp[0] = ck >> 8;
    tmp[1] = ck & 0xff;


    tmp_len = lent + 8;
    int send_to_cnt = 0;

    while (1) {
        send_to_cnt++;
        sendto(client, tmp, tmp_len, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
        clock_t begintime = clock();
        char recv[8];
        int lentmp = sizeof(serverAddr);
        int fail_send = 0;
        while (recvfrom(client, recv, 8, 0, (sockaddr*)&serverAddr, &lentmp) == SOCKET_ERROR)
            if (clock() - begintime > timeout) {
                fail_send = 1;
                if (send_to_cnt == 10)
                {
                    wave_hand();
                    return false;
                }
                break;
            }
        double sampleRTT = clock() - begintime;
        SHORT seq = (USHORT)((unsigned char)recv[6] << 8) + (USHORT)(unsigned char)recv[7];
        if (fail_send == 0 && checksum(recv, 8) == 0 && recv[2] == ACK && seq == curSeq) {
            addSampleRTT(sampleRTT);
            curSeq++;
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

DWORD WINAPI RecvThread(LPVOID lparam) {
    cout << "线程启动！！！！！！" << endl;
    int lastAckSeq = 1;
    while (true) {
        while (!timesaver.empty()) //时间队列有东西
        {
            SetPriorityClass(recver, HIGH_PRIORITY_CLASS);
            double start = timesaver.front(); //获取第一个时间也是最早的时间
            char recv[8];                     // 接收数组
            int lentmp = sizeof(clientAddr);
            bool flag = false;
            while (recvfrom(client, recv, 8, 0, (sockaddr*)&serverAddr, &lentmp) == SOCKET_ERROR) {
                if (clock() - start > timeout) {
                    //现在出现了时间超时
                    timesaver = queue<double>(); //队列清空
                    ssthresh = cwnd / 2;         // 阈值减半
                    cwnd = 1;                    // 窗口变成1
                    dupACKcount = 0;             // 收到的重复ACK计数为0
                    state = 0;                   // 状态变成慢启动状态(0)
                    curSeq = SendBase;           // 重传
                    flag = true;
                    cout << "超时啦" << curSeq << " " << SendBase << endl;
                    cout << "慢启动阶段 " << "窗口大小" << cwnd << " 阈值" << ssthresh << endl;
                    break;
                }
            }
            //若超时，时间队列被清空，跳出这层循环，直至等待时间队列有东西
            if (flag)
                break;

            double sampleRTT = clock() - start;
            // 没超时，收到了一个数据报
            SHORT seq = (USHORT)((unsigned char)recv[6] << 8) + (USHORT)(unsigned char)recv[7];
            // seq是收到报文的序号
            if (checksum(recv, 8) == 0 && recv[2] == ACK) {  // 校验和通过，并且是一个ACK回复
                if (seq == SendBase + 1) {
                    // 收到了正确的报文，窗口向后面移动一下
                    addSampleRTT(sampleRTT);
                    timesaver.pop();
                    lastAckSeq = seq;
                    SendBase++;

                    cout << "收到了 " << seq << "号报文的ACK" << endl;
                    ACKs[seq - 2] = true;   // 来一波确认

                    if (state == 0) {  // 慢启动状态
                        cwnd++;
                        dupACKcount = 0;
                        cout << "窗口大小:" << cwnd << " 阈值" << ssthresh << endl;
                    }
                    else if (state == 1) {  // 拥塞控制状态
                        cwnd += 1 / cwnd;
                        dupACKcount = 0;
                        cout << "窗口大小:" << cwnd << " 阈值" << ssthresh << endl;
                    }
                    else if (state == 2) { // 快速恢复状态
                        cwnd = ssthresh;
                        dupACKcount = 0;
                        state = 1;
                        cout << "拥塞避免阶段" << endl;
                        cout << "窗口大小:" << cwnd << " 阈值" << ssthresh << endl;
                    }
                    if (state == 0 && cwnd >= ssthresh) {    
                        // 如果cwnd大于等于阈值，状态转移，到拥塞控制状态
                        state = 1;
                        cout << "拥塞避免阶段" << endl;
                    }
                    
                    if (lastSeq == seq) {
                        closeall = true;
                        cout << "都收到了" << endl;
                        cout << curSeq << endl;
                        return 0;
                    }
                }
                else if (seq > SendBase + 1 && seq <= curSeq) {  
                    // 收到的编号，在累计确认情况下，收到的序号在窗口范围内
                    // 累计确认的情况下
                    addSampleRTT(sampleRTT);

                    int sub = seq - SendBase;
                    for (int i = 0; i < sub; i++) {
                        timesaver.pop();            // 弹出时间
                        ACKs[i + SendBase] = true;  // 写入记录ACK
                    }

                    SendBase = seq;

                    cout << "收到了 " << seq  << "号报文的ACK" << endl;
                    if (lastSeq == seq) {
                        closeall = true;
                        cout << "都收到了" << endl;
                        cout << curSeq << endl;
                        return 0;
                    }


                }
                else {
                    cout << "非正确回复，编号： " << seq << endl;
                    if (ACKs[seq - 2]) { // 考虑重复确认的情况
                        if (state == 2) { //在快速恢复的情况下
                            cwnd = cwnd + 1;
                            cout << "窗口大小:" << cwnd << " 阈值" << ssthresh << endl;
                        }
                        else {           // 其他两种情况下
                            dupACKcount++;   // dupAckcount加一
                        }
                    }
                    if (dupACKcount == 3 && state != 2) {   //出现三个重复ACK

                        ssthresh = cwnd / 2;
                        cwnd = ssthresh + 3.0;
                        state = 2;            // 状态转移到快速恢复
                        cout << "快速恢复阶段" << endl;
                        cout << "窗口大小:" << cwnd << " 阈值" << ssthresh << endl;
                        curSeq = SendBase;    // 窗口重发
                    }

                }
            }
            SetPriorityClass(recver, NORMAL_PRIORITY_CLASS);
        }
    }

}


void RENO_Send() {
    // 滑动窗口发送数据包
    cout << "慢启动阶段" << endl;
    cout << "窗口大小：" << cwnd << " 阈值" << ssthresh << endl;
    recver = CreateThread(NULL, NULL, RecvThread, (LPVOID)client, NULL, NULL);
    //创建接收线程
    while (true){
        while (curSeq - SendBase < (cwnd)){
            int last = curSeq + cwnd;
            if (last > lastSeq)
                last = lastSeq;

            for (int i = curSeq + 1; i <= last; i++) {
                curSeq = i;

                char* tmp = messages[i - startSeq];

                int tmp_len = ((unsigned char)tmp[4] << 8) + (unsigned char)tmp[5];

                tmp_len = tmp_len % 2 == 0 ? tmp_len : tmp_len + 1;


                USHORT seq = (USHORT)((unsigned char)tmp[6] << 8) + (USHORT)(unsigned char)tmp[7];
                int err = sendto(client, tmp, tmp_len, 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
                if (err == -1) {
                    cout << "发送错误" << seq;
                }

                cout << "发送 " << seq << "号数据报" << endl;
                timesaver.push(clock());
            }
            if (closeall)
                return;

        }
        if (closeall)
            return;
        //cout << "我还活着" << endl;
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


    _handle = _findfirst((File_Path + "*").c_str(), &info);
    if (_handle == -1)
    {
        cout << "failed to open directory..." << endl;
        return -1;
    }
    do {
        files.push_back(info.name);
    } while (!_findnext(_handle, &info));
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

    //现在buffer中有所有文件内容，len是长度
    //然后开始造包
    int package_num = len / Mlenx + (len % Mlenx != 0);//看要发送几个包

    ACKs = new bool[package_num];   // 有多少个包，就有多少个ACK确认帧
    memset(ACKs, false, sizeof(ACKs));     // 最开始全部初始化成 false
    cout << ACKs[0] << endl;
    // ACKs[0] 对应 seq==2 的 ACK 回复
    // 主要部分是在数据传输
    int seq = 2;
    startSeq = 2;

    for (int i = 0; i < package_num; i++) {

        bool last = (i == (package_num - 1));//是否是最后一个包
        int length = last ? len - (package_num - 1) * Mlenx : Mlenx;//数据的长度

        char* package = makeDatagram(buffer + i * Mlenx, length, last, seq);
        seq++;
        messages.push_back(package);
    }
    //到这一步，数据报全部准备完毕了

    cout << lastSeq <<" "<< package_num << endl;

    cout << "开始两次握手" << endl;
    shake_hand();
    cout << "两次握手完成，准备发送文件名" << endl;

    // 发送文件名也是，只要一个包就解决了，不需要滑动窗口，文件名总不能1M吧
    send_message((char*)(files[input + 2].c_str()), files[input + 2].length());
    cout << "文件名发送完成，准备发送文件内容" << endl;


    curSeq--;
    SendBase = curSeq;
    RENO_Send();
    curSeq++;
    cout << "文件内容发送完成，准备两次挥手" << endl;
    wave_hand();
    cout << "连接断开" << endl;
    closesocket(client);


    cout << timeout << endl;
    WSACleanup();
    return 0;
}