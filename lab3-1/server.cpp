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

    //���յ�һ��������Ϣ
    while (1)
    {
        if (recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) == -1)
        {
            return -1;
        }
        memcpy(&header, recvBuf, HEADER_LEN);
        if (header.flag == SYN && cksum((u_short*)&header, HEADER_LEN) == 0)
        {
            cout << "�յ���һ������: flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;
            break;
        }
    }

    //���͵ڶ���������Ϣ
    header.flag = SYN_ACK;
    header.checksum = 0;
    u_short temp = cksum((u_short*)&header, HEADER_LEN);
    header.checksum = temp;
    memcpy(sendBuf, &header, HEADER_LEN);
    if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    cout << "�ڶ������֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;
    clock_t start = clock();//��¼�ڶ������ַ���ʱ��

    //���յ���������
    while (recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
    {
        if (clock() - start > MAX_TIME)
        {
            if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;
            }
            cout << "TIME OUT:���ڽ����ش��ڶ�������" << endl;
        }
    }

    Header temp1;
    memcpy(&temp1, recvBuf, HEADER_LEN);
    if (temp1.flag == ACK && cksum((u_short*)&temp1, sizeof(temp1) == 0))
    {
        cout << "�յ�����������: flag=" << flag2str(int(temp1.flag)) << " У���=" << temp1.checksum << endl;
        cout << "�ɹ�����ͨ�ţ����Խ�������" << endl;
    }
    else
    {
        cout << "serve���ӷ��������������ͻ��ˣ�" << endl;
        return -1;
    }
    return 1;
}

int RecvMessage(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen, char* message)
{
    int fileLen= 0;//�ļ�����
    Header header;
    char* recvBuf = new char[MESSAGE_LEN];
    char* sendBuf = new char[MESSAGE_LEN];
    int index = 0;//��ǰȷ�ϵ��ķ������

    while (1)
    {
        int length = recvfrom(sockServ, recvBuf, MESSAGE_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//���ձ��ĳ���
        memcpy(&header, recvBuf, HEADER_LEN);
        //�ж��Ƿ��ǽ���
        if (header.flag == OVER && cksum((u_short*)&header, HEADER_LEN) == 0)
        {
            cout << "�ļ��������" << endl;
            break;
        }
        if(header.flag == unsigned char(0)&& cksum((u_short*)&header, HEADER_LEN) == 0)
        {
            if (index != int(header.index)&& cksum((u_short*)recvBuf, length - sizeof(header)))
            {
                //��Ȼ����index��ȷ�ϣ�������header.index��ȷ��
                header.flag = ACK;
                header.filelen = 0;
                header.index = (u_short)index;
                header.checksum = 0;
                u_short temp = cksum((u_short*)&header, HEADER_LEN);
                header.checksum = temp;
                memcpy(sendBuf, &header, HEADER_LEN);
                //�ط��ð���ACK
                sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen);
                cout << "�������ݰ���ȷ�� flag=" << flag2str(int(header.flag)) << " �������=" << (int)header.index << endl;
                continue;//���������ݰ�
            }
            index = int(header.index);
            index %= 65536;
            //ȡ��recvBuf�е�����
            cout << "���շ��� " << length - HEADER_LEN << " bytes flag=" << flag2str(int(header.flag)) << "������� = "
                << (int)header.index << " У���=" << int(header.checksum) << endl;
            char* temp = new char[length - HEADER_LEN];
            memcpy(temp, recvBuf + HEADER_LEN, length - HEADER_LEN);
            memcpy(message + fileLen, temp, length - HEADER_LEN);
            fileLen = fileLen + int(header.filelen);

            //����ACK
            header.flag = ACK;
            header.filelen = 0;
            header.index = (u_short)index;
            header.checksum = 0;
            u_short temp1 = cksum((u_short*)&header, HEADER_LEN);
            header.checksum = temp1;
            memcpy(sendBuf, &header, HEADER_LEN);
            //�ط��ð���ACK
            sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen);
            cout << "�������ݰ���ȷ�� flag=" << flag2str(int(header.flag)) << " �������=" << (int)header.index << endl;
            index++;//ȷ�ϸð�֮�󣬲�����һ����
            index %= 65536;
        }
    }
    //����OVER��Ϣ
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
//��ȥ�ٸ�
int waveHand(SOCKET& sockServ, SOCKADDR_IN& ClientAddr, int& ClientAddrLen)
{
    Header header;
    char* recvBuf = new char[HEADER_LEN];
    char* sendBuf = new char[HEADER_LEN];
    while (1)
    {
        int length = recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen);//���ձ��ĳ���
        memcpy(&header, recvBuf, HEADER_LEN);
        if (header.flag == FIN && cksum((u_short*)&header, HEADER_LEN) == 0)
        {
            cout << "�ɹ����յ�һ�λ�����Ϣ" << endl;
            break;
        }
    }
    //���͵ڶ��λ�����Ϣ
    header.flag = ACK;
    header.checksum = 0;
    u_short temp = cksum((u_short*)&header, HEADER_LEN);
    header.checksum = temp;
    memcpy(sendBuf, &header, HEADER_LEN);
    if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    cout << "�ڶ��λ��֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;

    //�ڶ��η��͵�ACK�����ܻᶪʧ�����ǽ������´���
    clock_t start = clock();//��¼�ڶ��λ��ַ���ʱ��
    u_long mode = 1;
    ioctlsocket(sockServ, FIONBIO, &mode);
    while (1)
    {
        if (int(clock() - start) > int(MAX_TIME))//�ȴ�MAX_TIME
            break;
        if (recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) > 0)//���յ�����
        {
            cout << "�����ж�1" << endl;
            memcpy(&header, recvBuf, HEADER_LEN);
            if (header.flag == FIN && cksum((u_short*)&header, HEADER_LEN) == 0)
            {
                cout << "ACK���Ķ�ʧ���յ��ͻ��˵�һ�λ����ش�" << endl;
                break;
            }
            //���͵ڶ��λ�����Ϣ
            header.flag = ACK;
            header.checksum = 0;
            u_short temp = cksum((u_short*)&header, HEADER_LEN);
            header.checksum = temp;
            memcpy(sendBuf, &header, HEADER_LEN);
            if (sendto(sockServ, sendBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;
            }
            cout << "�ڶ��λ��֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;
            start = clock();//�ٴν���ȴ�2MSL
        }
    }

    //�����λ���
    header.flag = FIN_ACK;
    header.checksum = 0;
    header.checksum = cksum((u_short*)&header, HEADER_LEN);//����У���
    if (sendto(sockServ, (char*)&header, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
    {
        return -1;
    }
    cout << "�����λ��֣�flag=" << flag2str(int(header.flag)) << " У���=" << header.checksum << endl;
    start = clock();//�����ʱҪ�����ش�

    //���յ��Ĵλ���
    while (recvfrom(sockServ, recvBuf, HEADER_LEN, 0, (sockaddr*)&ClientAddr, &ClientAddrLen) <= 0)
    {
        if (clock() - start > MAX_TIME)
        {
            if (sendto(sockServ, (char*)&header, HEADER_LEN, 0, (sockaddr*)&ClientAddr, ClientAddrLen) == -1)
            {
                return -1;
            }
            cout << "�����λ��ֳ�ʱ�����ڽ����ش�" << endl;
        }
    }

    Header temp1;
    memcpy(&temp1, recvBuf, HEADER_LEN);
    if (temp1.flag == ACK && cksum((u_short*)&temp1, sizeof(temp1)) == 0)
    {
        cout << "�ɹ����յ��Ĵλ���" << endl;
    }
    else
    {
        cout << "��������,�ͻ��˹رգ�" << endl;
        return -1;
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
    server_addr.sin_addr.s_addr = htonl(2130706433);

    server = socket(AF_INET, SOCK_DGRAM, 0);
    bind(server, (SOCKADDR*)&server_addr, sizeof(server_addr));//���׽��֣��������״̬
    cout << "�������״̬���ȴ��ͻ�������" << endl;
    int len = sizeof(server_addr);

    //��������
    shakeHand(server, server_addr, len);
    cout << " **********************************************************" << endl;
    char* name = new char[20];
    char* data = new char[100000000];
    int namelen = RecvMessage(server, server_addr, len, name);
    cout << "���ֽ������" << endl;
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
    cout <<a<<"�ļ��ѳɹ����ص�����server-receive-file�ļ�����" << endl;

    system("pause");
    return 0;
}
