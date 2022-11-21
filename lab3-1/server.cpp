#include <iostream>
#include <WINSOCK2.h>
#include <time.h>
#include <fstream>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

#define MESSAGE_LEN 1036 //message length
#define HEADER_LEN 12
#define FILE_LEN 1024 //valid file length
#define MAX_TIME 0.5*CLOCKS_PER_SEC

//FIN,ACK,SYN high->low
#define SYN 0x1 //001
#define ACK 0x2 //010
#define SYN_ACK 0x3 //011
#define FIN 0x4 //100
#define FIN_ACK 0x6 //110
#define OVER 0x7//111

struct Header {
    
	//checksum
	u_short checksum = 0;//16 bits
	//group index
	u_short index = 0;
	//file length
	unsigned int filelen = 0;
	//flags
	unsigned char flag = 0;
};
string flag2str(int flag)
{
    switch (flag)
    {
    case 0:
        return "NULL";
    case SYN:
        return "SYN";
    case ACK:
        return "ACK";
    case SYN_ACK:
        return "SYN_ACK";
    case FIN:
        return "FIN";
    case FIN_ACK:
        return "FIN_ACK";
    case OVER:
        return "OVER";
    default:
        break;
    }
}
//calculate checksum
u_short cksum(u_short* mes, int size) {
	int count = (size + 1) / 2;
	u_short* buf = (u_short*)malloc(size + 1);
	memset(buf, 0, size + 1);
	memcpy(buf, mes, size);
	u_long sum = 0;
	while (count--) {
		sum += *buf++;
		if (sum & 0xffff0000) {
			sum &= 0xffff;
			sum++;
		}
	}
	return ~(sum & 0xffff);
}

int shakeHand(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen)
{

    Header header;
    char* recvBuf = new char[HEADER_LEN];
    char* sendBuf = new char[HEADER_LEN];

    //接收第一次握手信息
    while (1)
    {
        if (recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) == -1)
        {
            return -1;
        }
        memcpy(&header, recvBuf, HEADER_LEN);
        if (header.flag == SYN && cksum((u_short*)&header, HEADER_LEN) == 0)
        {
            cout << "收到第一次握手: flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;
            break;
        }
    }

    //发送第二次握手信息
    header.flag = SYN_ACK;
    header.checksum = 0;
    u_short temp = cksum((u_short*)&header, HEADER_LEN);
    header.checksum = temp;
    memcpy(sendBuf, &header, HEADER_LEN);
    if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    cout << "第二次握手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;
    clock_t start = clock();//记录第二次握手发送时间

    //接收第三次握手
    while (recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
    {
        if (clock() - start > MAX_TIME)
        {
            if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;
            }
            cout << "TIME OUT:正在进行重传第二次握手" << endl;
        }
    }

    Header temp1;
    memcpy(&temp1, recvBuf, HEADER_LEN);
    if (temp1.flag == ACK && cksum((u_short*)&temp1, sizeof(temp1) == 0))
    {
        cout << "收到第三次握手: flag=" << flag2str(int(temp1.flag)) << " 校验和=" << temp1.checksum << endl;
        cout << "成功建立通信！可以接收数据" << endl;
    }
    else
    {
        cout << "serve连接发生错误，请重启客户端！" << endl;
        return -1;
    }
    return 1;
}

int RecvMessage(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen, char* message)
{
    int fileLen= 0;//文件长度
    Header header;
    char* recvBuf = new char[MESSAGE_LEN];
    char* sendBuf = new char[MESSAGE_LEN];
    int index = 0;//当前确认到的分组序号

    while (1)
    {
        int length = recvfrom(sockServ, recvBuf, MESSAGE_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//接收报文长度
        memcpy(&header, recvBuf, HEADER_LEN);
        //判断是否是结束
        if (header.flag == OVER && cksum((u_short*)&header, HEADER_LEN) == 0)
        {
            cout << "文件接收完毕" << endl;
            break;
        }
        if(header.flag == unsigned char(0)&& cksum((u_short*)&header, HEADER_LEN) == 0)
        {
            if (index != int(header.index)&& cksum((u_short*)recvBuf, length - sizeof(header)))
            {
                //依然返回index的确认，而不是header.index的确认
                header.flag = ACK;
                header.filelen = 0;
                header.index = (u_short)index;
                header.checksum = 0;
                u_short temp = cksum((u_short*)&header, HEADER_LEN);
                header.checksum = temp;
                memcpy(sendBuf, &header, HEADER_LEN);
                //重发该包的ACK
                sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen);
                cout << "接收数据包的确认 flag=" << flag2str(int(header.flag)) << " 分组序号=" << (int)header.index << endl;
                continue;//丢弃该数据包
            }
            index = int(header.index);
            index %= 65536;
            //取出recvBuf中的内容
            cout << "接收分组 " << length - HEADER_LEN << " bytes flag=" << flag2str(int(header.flag)) << "分组序号 = "
                << (int)header.index << " 校验和=" << int(header.checksum) << endl;
            char* temp = new char[length - HEADER_LEN];
            memcpy(temp, recvBuf + HEADER_LEN, length - HEADER_LEN);
            memcpy(message + fileLen, temp, length - HEADER_LEN);
            fileLen = fileLen + int(header.filelen);

            //返回ACK
            header.flag = ACK;
            header.filelen = 0;
            header.index = (u_short)index;
            header.checksum = 0;
            u_short temp1 = cksum((u_short*)&header, HEADER_LEN);
            header.checksum = temp1;
            memcpy(sendBuf, &header, HEADER_LEN);
            //重发该包的ACK
            sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen);
            cout << "接收数据包的确认 flag=" << flag2str(int(header.flag)) << " 分组序号=" << (int)header.index << endl;
            index++;//确认该包之后，才能下一个包
            index %= 65536;
        }
    }
    //发送OVER信息
    header.flag = OVER;
    header.checksum = 0;
    u_short temp = cksum((u_short*)&header, HEADER_LEN);
    header.checksum = temp;
    memcpy(sendBuf, &header, HEADER_LEN);
    if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    return fileLen;
}
//下去再改
int waveHand(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen)
{
    Header header;
    char* recvBuf = new char[HEADER_LEN];
    char* sendBuf = new char[HEADER_LEN];
    while (1)
    {
        int length = recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//接收报文长度
        memcpy(&header, recvBuf, HEADER_LEN);
        if (header.flag == FIN && cksum((u_short*)&header, HEADER_LEN) == 0)
        {
            cout << "成功接收第一次挥手信息" << endl;
            break;
        }
    }
    //发送第二次挥手信息
    header.flag = ACK;
    header.checksum = 0;
    u_short temp = cksum((u_short*)&header, HEADER_LEN);
    header.checksum = temp;
    memcpy(sendBuf, &header, HEADER_LEN);
    if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    cout << "第二次挥手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;

    //第二次发送的ACK包可能会丢失，我们进行如下处理
    clock_t start = clock();//记录第二次挥手发送时间
    u_long mode = 1;
    ioctlsocket(sockServ, FIONBIO, &mode);
    while (1)
    {
        if (int(clock() - start) > int(MAX_TIME))//等待MAX_TIME
            break;
        if (recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) > 0)//接收到报文
        {
            cout << "进入判断1" << endl;
            memcpy(&header, recvBuf, HEADER_LEN);
            if (header.flag == FIN && cksum((u_short*)&header, HEADER_LEN) == 0)
            {
                cout << "ACK报文丢失，收到客户端第一次挥手重传" << endl;
                break;
            }
            //发送第二次挥手信息
            header.flag = ACK;
            header.checksum = 0;
            u_short temp = cksum((u_short*)&header, HEADER_LEN);
            header.checksum = temp;
            memcpy(sendBuf, &header, HEADER_LEN);
            if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;
            }
            cout << "第二次挥手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;
            start = clock();//再次进入等待2MSL
        }
    }

    //第三次挥手
    header.flag = FIN_ACK;
    header.checksum = 0;
    header.checksum = cksum((u_short*)&header, HEADER_LEN);//计算校验和
    if (sendto(sockServ, (char*)&header, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    cout << "第三次挥手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;
    start = clock();//如果超时要进行重传

    //接收第四次挥手
    while (recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
    {
        if (clock() - start > MAX_TIME)
        {
            if (sendto(sockServ, (char*)&header, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;
            }
            cout << "第三次挥手超时，正在进行重传" << endl;
        }
    }

    Header temp1;
    memcpy(&temp1, recvBuf, HEADER_LEN);
    if (temp1.flag == ACK && cksum((u_short*)&temp1, sizeof(temp1)) == 0)
    {
        cout << "成功接收第四次挥手" << endl;
    }
    else
    {
        cout << "发生错误,客户端关闭！" << endl;
        return -1;
    }
    cout << "四次挥手结束，连接断开！" << endl;
    return 1;
}


int main()
{
    WSADATA wsadata;
    WSAStartup(MAKEWORD(2, 2), &wsadata);

    SOCKADDR_IN server_addr;
    SOCKET server;

    server_addr.sin_family = AF_INET;//使用IPV4
    server_addr.sin_port = htons(3456);
    server_addr.sin_addr.s_addr = htonl(2130706433);

    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));//绑定套接字，进入监听状态
    cout << "进入监听状态，等待客户端上线" << endl;
    int len = sizeof(server_addr);

    //建立连接
    shakeHand(server, server_addr, len);
    cout << " **********************************************************" << endl;
    char* name = new char[20];
    char* data = new char[100000000];
    int namelen = RecvMessage(server, server_addr, len, name);
    cout << "名字接收完毕" << endl;
    cout << " **********************************************************" << endl;
    int datalen = RecvMessage(server, server_addr, len, data);
    string pathname=".\\server-receive-file\\";
    string a = "";
    for (int i = 0; i < namelen; i++)
    {
        a = a + name[i];
    }
    cout << " **********************************************************" << endl;
    waveHand(server, server_addr, len);
    pathname += a;
    ofstream fout(pathname.c_str(), ofstream::binary);
    for (int i = 0; i < datalen; i++)
    {
        fout << data[i];
    }
    fout.close();
    cout <<a<<"文件已成功下载到本地server-receive-file文件夹下" << endl;

    system("pause");
    return 0;
}
