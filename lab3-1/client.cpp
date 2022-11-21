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

string toFilename(int op)
{
	switch (op)
	{
	case 1:
		return "1.jpg";
	case 2:
		return "2.jpg";
	case 3:
		return "3.jpg";
	case 4:
		return "helloworld.txt";
	default:
		break;
	}
}

struct Header {
	//checksum
	u_short checksum = 0;//16 bits
	//group index
	u_short index=0;
	//file length
	unsigned int filelen = 0;
	//flags
	unsigned char flag = 0;
};

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
//3 way hand shake to create connection
int shakeHand(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
	Header header;
	//set flags
	header.flag = SYN;
	header.checksum = 0;
	u_short temp = cksum((u_short*)&header, HEADER_LEN);
	header.checksum = temp;

	char* sendBuf = new char[HEADER_LEN];//12
	memcpy(sendBuf, &header, HEADER_LEN);//将首部放入缓冲区
	
	if (sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	
	cout << "第一次握手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;

	clock_t start = clock(); //time of first shake hand

	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode);

	//receive second shake hand
	char* recvBuf = new char[HEADER_LEN];//12
	while (recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//overtime,repeat first shake hand
		{
			//sendBuf has not changed
			sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "TIME OUT:正在进行重传第一次握手" << endl;
		}
	}

	//according to recvBuf to verify checksum
	memcpy(&header, recvBuf, HEADER_LEN);
	if (header.flag == SYN_ACK && cksum((u_short*)&header, HEADER_LEN) == 0)
	{
		cout << "收到第二次握手: flag=" << flag2str(int(header.flag))<<" 校验和="<< header.checksum << endl;
	}
	else
	{
		cout << "连接发生错误，请重启客户端！" << endl;
		return -1;
	}

	//third shake hand
	header.flag = ACK;
	header.checksum = 0;
	header.checksum = cksum((u_short*)&header, HEADER_LEN);
	if (sendto(socketClient, (char*)&header, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	cout << "第三次握手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;
	cout << "服务器成功连接！可以发送数据" << endl;
	return 1;
}

//send single data package
//index:group index
void send_package(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len, int& index)
{
	char* sendBuf = new char[MESSAGE_LEN];
	Header header;
	header.filelen = len;
	header.index = (u_short)index;

	memcpy(sendBuf, &header, HEADER_LEN);
	
	memcpy(sendBuf + HEADER_LEN, message, len);
	u_short checksum = cksum((u_short*)sendBuf, HEADER_LEN);//计算校验和
	header.checksum = checksum;
	memcpy(sendBuf, &header, HEADER_LEN);
	sendto(socketClient, sendBuf, len + HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);//发送
	cout << "发送分组 " << len << " bytes" << " flag=" << flag2str(int(header.flag)) << " 分组序号="
		<< int(header.index) << " 校验和=" << int(header.checksum) << endl;
	clock_t start = clock();//记录发送时间

	 //接收确认信息
	char* recvBuf = new char[HEADER_LEN];
	while (1)
	{
		u_long mode = 1;
		ioctlsocket(socketClient, FIONBIO, &mode);
		while (recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
		{
			if (clock() - start > MAX_TIME)
			{
				sendto(socketClient, sendBuf, len + HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);//发送
				cout << "TIME OUT:重新发送分组 " << len << " bytes" << " flag = " << flag2str(int(header.flag)) << 
					" 分组序号 ="<< int(header.index) << " 校验和=" << int(header.checksum) << endl;
				start = clock();//记录发送时间
			}
		}
		Header recvHeader;
		memcpy(&recvHeader, recvBuf, sizeof(recvHeader));
		u_short checksum = cksum((u_short*)&recvHeader, sizeof(recvHeader));
		if (recvHeader.index == u_short(index) && recvHeader.flag == ACK)
		{
			cout << "分组已被确认 flag=" << flag2str(int(recvHeader.flag)) << " 分组序号=" << int(recvHeader.index) << endl;
			break;
		}
		else//直到接收到确认信息，否则继续等待接收，继续重传分组
		{
			continue;
		}
	}
	u_long mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式
}

void send(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len)
{
	int packagenum = len / FILE_LEN + (len % FILE_LEN != 0);
	int index = 0;//分组编号
	for (int i = 0; i < packagenum; i++)
	{
		send_package(socketClient, servAddr, servAddrlen, message + i * FILE_LEN, 
			i == packagenum - 1 ? len - (packagenum - 1) * FILE_LEN : FILE_LEN, index);
		index++;
		index %= 65536;
	}
	//发送结束信息
	Header header;
	char* sendBuf = new char[HEADER_LEN];
	header.flag = OVER;
	header.checksum = 0;
	u_short temp = cksum((u_short*)&header, HEADER_LEN);
	header.checksum= temp;
	memcpy(sendBuf, &header, HEADER_LEN);
	sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);
	cout << "发送END信息!" << endl;
	clock_t start = clock();

	//接收反馈
	char* recvBuf = new char[HEADER_LEN];
	while (1)
	{
		u_long mode = 1;
		ioctlsocket(socketClient, FIONBIO, &mode);
		while (recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
		{
			if (clock() - start > MAX_TIME)
			{
				sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);
				cout << "TIME OUT:重新发送END信息" << endl;
				start = clock();
			}
		}
		memcpy(&header, recvBuf, HEADER_LEN);
		u_short checksum = cksum((u_short*)&header, HEADER_LEN);
		if (header.flag == OVER)
		{
			cout << "对方已成功接收文件!" << endl;
			break;
		}
		else
		{
			continue;
		}
	}
	u_long mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//改回阻塞模式
}

int wave_hand(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
	Header header;
	char* sendBuf = new char[HEADER_LEN];
	char* recvBuf = new char[HEADER_LEN];

	//第一次挥手
	header.flag = FIN;
	header.checksum = 0;//校验和置0
	u_short temp = cksum((u_short*)&header, HEADER_LEN);
	header.checksum = temp;//计算校验和
	memcpy(sendBuf, &header, HEADER_LEN);//将首部放入缓冲区
	if (sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	cout << "第一次挥手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;
	clock_t start = clock(); //记录发送第一次挥手时间

	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode);

	//接收第二次挥手
	while (recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//超时，重新传输第一次挥手
		{
			sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "第一次挥手超时，正在进行重传" << endl;
		}
	}

	//进行校验和检验
	memcpy(&header, recvBuf, HEADER_LEN);
	if (header.flag == ACK && cksum((u_short*)&header, HEADER_LEN) == 0)
	{
		cout << "收到第二次挥手信息" << endl;
	}
	else
	{
		cout << "连接发生错误，程序直接退出！" << endl;
		return -1;
	}

	//接收第三次挥手
	while (1)
	{
		int length = recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen);//接收报文长度
		memcpy(&header, recvBuf, HEADER_LEN);
		if (header.flag == FIN_ACK && cksum((u_short*)&header, HEADER_LEN) == 0)
		{
			cout << "成功接收第三次挥手信息" << endl;
			break;
		}
	}
	//发送第四次挥手
	header.flag = ACK;
	header.checksum = 0;
	temp = cksum((u_short*)&header, HEADER_LEN);
	header.checksum = temp;
	memcpy(sendBuf, &header, HEADER_LEN);
	if (sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	cout << "第四次挥手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;

	start = clock();
	while (1)
	{
		if (clock() - start > MAX_TIME)//等待MAX_TIME
			break;
		if (recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen)>0)//接收到报文
		{
			memcpy(&header, recvBuf, HEADER_LEN);
			if (header.flag == FIN_ACK && cksum((u_short*)&header, HEADER_LEN) == 0)
			{
				cout << "ACK报文丢失，收到服务端第三次挥手重传" << endl;
				break;
			}
			//发送第四次挥手信息
			header.flag = ACK;
			header.checksum = 0;
			temp = cksum((u_short*)&header, HEADER_LEN);
			header.checksum = temp;
			memcpy(sendBuf, &header, HEADER_LEN);
			if (sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
			{
				return -1;
			}
			cout << "第四次挥手：flag=" << flag2str(int(header.flag)) << " 校验和=" << header.checksum << endl;
			start = clock();//再次进入等待2MSL
		}
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
	server_addr.sin_addr.s_addr = htonl(2130706433);//127.0.0.1

	server = socket(AF_INET, SOCK_DGRAM, 0);
	int len = sizeof(server_addr);
	//建立连接
	cout <<  " **********************************************************"<< endl;
	if (shakeHand(server, server_addr, len) == -1)
	{
		return 0;
	}
	cout << " **********************************************************" << endl;

	string pathname=".\\client-send-file\\";
	int opfile = 0;
	cout << "请输入你要测试的文件" << endl;
	cout << "1:1.jpg 2:2.jpg 3:3.jpg 4:helloworld.txt" << endl;
	cin >> opfile;
	string filename = toFilename(opfile);
	pathname += filename;
	ifstream fin(pathname.c_str(), ifstream::binary);//以二进制方式打开文件
	char* buffer = new char[100000000];
	int cnt = 0;
	unsigned char temp = fin.get();
	while (fin)
	{
		buffer[cnt++] = temp;
		temp = fin.get();
	}
	fin.close();
	cout << "文件长度=" << cnt << endl;
	send(server, server_addr, len, (char*)(filename.c_str()), filename.length());//先把文件名发过去
	clock_t start = clock();
	send(server, server_addr, len, buffer, cnt);
	clock_t end = clock();
	cout << "传输总时间为:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
	cout << "吞吐率为:" << ((float)cnt) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
	cout << " **********************************************************" << endl;
	wave_hand(server, server_addr, len);
	system("pause");
	return 0;
}



