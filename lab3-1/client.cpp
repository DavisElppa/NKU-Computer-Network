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
	memcpy(sendBuf, &header, HEADER_LEN);//���ײ����뻺����
	
	if (sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	
	cout << "��һ�����֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;

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
			cout << "TIME OUT:���ڽ����ش���һ������" << endl;
		}
	}

	//according to recvBuf to verify checksum
	memcpy(&header, recvBuf, HEADER_LEN);
	if (header.flag == SYN_ACK && cksum((u_short*)&header, HEADER_LEN) == 0)
	{
		cout << "�յ��ڶ�������: flag=" << flag2str(int(header.flag))<<" У���="<< header.checksum << endl;
	}
	else
	{
		cout << "���ӷ��������������ͻ��ˣ�" << endl;
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
	cout << "���������֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;
	cout << "�������ɹ����ӣ����Է�������" << endl;
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
	u_short checksum = cksum((u_short*)sendBuf, HEADER_LEN);//����У���
	header.checksum = checksum;
	memcpy(sendBuf, &header, HEADER_LEN);
	sendto(socketClient, sendBuf, len + HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);//����
	cout << "���ͷ��� " << len << " bytes" << " flag=" << flag2str(int(header.flag)) << " �������="
		<< int(header.index) << " У���=" << int(header.checksum) << endl;
	clock_t start = clock();//��¼����ʱ��

	 //����ȷ����Ϣ
	char* recvBuf = new char[HEADER_LEN];
	while (1)
	{
		u_long mode = 1;
		ioctlsocket(socketClient, FIONBIO, &mode);
		while (recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
		{
			if (clock() - start > MAX_TIME)
			{
				sendto(socketClient, sendBuf, len + HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);//����
				cout << "TIME OUT:���·��ͷ��� " << len << " bytes" << " flag = " << flag2str(int(header.flag)) << 
					" ������� ="<< int(header.index) << " У���=" << int(header.checksum) << endl;
				start = clock();//��¼����ʱ��
			}
		}
		Header recvHeader;
		memcpy(&recvHeader, recvBuf, sizeof(recvHeader));
		u_short checksum = cksum((u_short*)&recvHeader, sizeof(recvHeader));
		if (recvHeader.index == u_short(index) && recvHeader.flag == ACK)
		{
			cout << "�����ѱ�ȷ�� flag=" << flag2str(int(recvHeader.flag)) << " �������=" << int(recvHeader.index) << endl;
			break;
		}
		else//ֱ�����յ�ȷ����Ϣ����������ȴ����գ������ش�����
		{
			continue;
		}
	}
	u_long mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ
}

void send(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen, char* message, int len)
{
	int packagenum = len / FILE_LEN + (len % FILE_LEN != 0);
	int index = 0;//������
	for (int i = 0; i < packagenum; i++)
	{
		send_package(socketClient, servAddr, servAddrlen, message + i * FILE_LEN, 
			i == packagenum - 1 ? len - (packagenum - 1) * FILE_LEN : FILE_LEN, index);
		index++;
		index %= 65536;
	}
	//���ͽ�����Ϣ
	Header header;
	char* sendBuf = new char[HEADER_LEN];
	header.flag = OVER;
	header.checksum = 0;
	u_short temp = cksum((u_short*)&header, HEADER_LEN);
	header.checksum= temp;
	memcpy(sendBuf, &header, HEADER_LEN);
	sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);
	cout << "����END��Ϣ!" << endl;
	clock_t start = clock();

	//���շ���
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
				cout << "TIME OUT:���·���END��Ϣ" << endl;
				start = clock();
			}
		}
		memcpy(&header, recvBuf, HEADER_LEN);
		u_short checksum = cksum((u_short*)&header, HEADER_LEN);
		if (header.flag == OVER)
		{
			cout << "�Է��ѳɹ������ļ�!" << endl;
			break;
		}
		else
		{
			continue;
		}
	}
	u_long mode = 0;
	ioctlsocket(socketClient, FIONBIO, &mode);//�Ļ�����ģʽ
}

int wave_hand(SOCKET& socketClient, SOCKADDR_IN& servAddr, int& servAddrlen)
{
	Header header;
	char* sendBuf = new char[HEADER_LEN];
	char* recvBuf = new char[HEADER_LEN];

	//��һ�λ���
	header.flag = FIN;
	header.checksum = 0;//У�����0
	u_short temp = cksum((u_short*)&header, HEADER_LEN);
	header.checksum = temp;//����У���
	memcpy(sendBuf, &header, HEADER_LEN);//���ײ����뻺����
	if (sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	cout << "��һ�λ��֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;
	clock_t start = clock(); //��¼���͵�һ�λ���ʱ��

	u_long mode = 1;
	ioctlsocket(socketClient, FIONBIO, &mode);

	//���յڶ��λ���
	while (recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen) <= 0)
	{
		if (clock() - start > MAX_TIME)//��ʱ�����´����һ�λ���
		{
			sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen);
			start = clock();
			cout << "��һ�λ��ֳ�ʱ�����ڽ����ش�" << endl;
		}
	}

	//����У��ͼ���
	memcpy(&header, recvBuf, HEADER_LEN);
	if (header.flag == ACK && cksum((u_short*)&header, HEADER_LEN) == 0)
	{
		cout << "�յ��ڶ��λ�����Ϣ" << endl;
	}
	else
	{
		cout << "���ӷ������󣬳���ֱ���˳���" << endl;
		return -1;
	}

	//���յ����λ���
	while (1)
	{
		int length = recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen);//���ձ��ĳ���
		memcpy(&header, recvBuf, HEADER_LEN);
		if (header.flag == FIN_ACK && cksum((u_short*)&header, HEADER_LEN) == 0)
		{
			cout << "�ɹ����յ����λ�����Ϣ" << endl;
			break;
		}
	}
	//���͵��Ĵλ���
	header.flag = ACK;
	header.checksum = 0;
	temp = cksum((u_short*)&header, HEADER_LEN);
	header.checksum = temp;
	memcpy(sendBuf, &header, HEADER_LEN);
	if (sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
	{
		return -1;
	}
	cout << "���Ĵλ��֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;

	start = clock();
	while (1)
	{
		if (clock() - start > MAX_TIME)//�ȴ�MAX_TIME
			break;
		if (recvfrom(socketClient, recvBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, &servAddrlen)>0)//���յ�����
		{
			memcpy(&header, recvBuf, HEADER_LEN);
			if (header.flag == FIN_ACK && cksum((u_short*)&header, HEADER_LEN) == 0)
			{
				cout << "ACK���Ķ�ʧ���յ�����˵����λ����ش�" << endl;
				break;
			}
			//���͵��Ĵλ�����Ϣ
			header.flag = ACK;
			header.checksum = 0;
			temp = cksum((u_short*)&header, HEADER_LEN);
			header.checksum = temp;
			memcpy(sendBuf, &header, HEADER_LEN);
			if (sendto(socketClient, sendBuf, HEADER_LEN, 0, (sockaddr*)&servAddr, servAddrlen) == -1)
			{
				return -1;
			}
			cout << "���Ĵλ��֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;
			start = clock();//�ٴν���ȴ�2MSL
		}
	}

	cout << "�Ĵλ��ֽ��������ӶϿ���" << endl;
	return 1;
}

int main()
{
	WSADATA wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	SOCKADDR_IN server_addr;
	SOCKET server;

	server_addr.sin_family = AF_INET;//ʹ��IPV4
	server_addr.sin_port = htons(3456);
	server_addr.sin_addr.s_addr = htonl(2130706433);//127.0.0.1

	server = socket(AF_INET, SOCK_DGRAM, 0);
	int len = sizeof(server_addr);
	//��������
	cout <<  " **********************************************************"<< endl;
	if (shakeHand(server, server_addr, len) == -1)
	{
		return 0;
	}
	cout << " **********************************************************" << endl;

	string pathname=".\\client-send-file\\";
	int opfile = 0;
	cout << "��������Ҫ���Ե��ļ�" << endl;
	cout << "1:1.jpg 2:2.jpg 3:3.jpg 4:helloworld.txt" << endl;
	cin >> opfile;
	string filename = toFilename(opfile);
	pathname += filename;
	ifstream fin(pathname.c_str(), ifstream::binary);//�Զ����Ʒ�ʽ���ļ�
	char* buffer = new char[100000000];
	int cnt = 0;
	unsigned char temp = fin.get();
	while (fin)
	{
		buffer[cnt++] = temp;
		temp = fin.get();
	}
	fin.close();
	cout << "�ļ�����=" << cnt << endl;
	send(server, server_addr, len, (char*)(filename.c_str()), filename.length());//�Ȱ��ļ�������ȥ
	clock_t start = clock();
	send(server, server_addr, len, buffer, cnt);
	clock_t end = clock();
	cout << "������ʱ��Ϊ:" << (end - start) / CLOCKS_PER_SEC << "s" << endl;
	cout << "������Ϊ:" << ((float)cnt) / ((end - start) / CLOCKS_PER_SEC) << "byte/s" << endl;
	cout << " **********************************************************" << endl;
	wave_hand(server, server_addr, len);
	system("pause");
	return 0;
}



