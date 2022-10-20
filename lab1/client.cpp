/*
* Ԥ��ʵ�ֵĸ��ӹ���
* ����������--ͨ�������ת��
* ���߳�
*/
#include<iostream>
#include<WinSock2.h>
#include<string>
#include<WS2tcpip.h>
#include<thread>
#pragma comment(lib,"ws2_32.lib")

#define MESSAGE_LEN 256
#define CONNECT_PORT 33414

using namespace std;

int flag = 1;//�ж��Ƿ��˳�
char user_name[20];

DWORD WINAPI Recv(LPVOID lparam_socket) {
	int isRecv;//�ж��Ƿ���յ���Ϣ
	SOCKET* recvSocket = (SOCKET*)lparam_socket;//ʹ��ָ���ԭ��Ϊ����Ҫָ������socket�ĵ�ַ
	while (1)
	{
		char recvMessage[MESSAGE_LEN];
		isRecv = recv(*recvSocket, recvMessage, MESSAGE_LEN, 0);
		if (isRecv > 0 && flag == 1)//���յ���Ϣ����û���˳�Ⱥ�ģ���Ϊ��˫�̣߳�������Ҫ�ж�flag
		{
			SYSTEMTIME time;
			GetLocalTime(&time);
			cout << time.wYear << ":" << time.wMonth << ":"
				<< time.wDay << ":" << time.wHour << ":"
				<< time.wMinute << time.wSecond << endl;
			cout << "���յ���Ϣ:" << recvMessage << endl;
			cout << "-----------------------------" << endl;
			cout << "��������Ҫ���͵���Ϣ" << endl;
		}
		else
		{
			closesocket(*recvSocket);
			return 0;
		}
	}
	
	
}
DWORD WINAPI Send(LPVOID lparam_socket) {
	int isSend;
	SOCKET* sendSocket = (SOCKET*)lparam_socket;//ʹ��ָ���ԭ��Ϊ����Ҫָ������socket�ĵ�ַ

	while (1)
	{
		cout << "��������Ҫ���͵���Ϣ" << endl;
		char sendMessage[MESSAGE_LEN];
		cin.getline(sendMessage, MESSAGE_LEN);//getline����ʶ��ո����������Զ�����

		if (string(sendMessage) == "quit")//��ʾ���û�Ҫ�˳�Ⱥ��
		{
			flag = 0;
			closesocket(*sendSocket);
			WSACleanup();//����WSACleanup�����������Socket��İ󶨲����ͷ�Socket����ռ�õ�ϵͳ��Դ
			return 0;
		}
		else
		{
			isSend = send(*sendSocket, sendMessage, MESSAGE_LEN, 0);
			if (isSend == SOCKET_ERROR)//-1��ʾ����
			{
				cout << "������Ϣʧ��:" << WSAGetLastError()<<endl;
				closesocket(*sendSocket);
				WSACleanup();//����WSACleanup�����������Socket��İ󶨲����ͷ�Socket����ռ�õ�ϵͳ��Դ
				return 0;
			}
			else
			{
				SYSTEMTIME time;
				GetLocalTime(&time);
				cout << time.wYear << ":" << time.wMonth << ":"
					<< time.wDay << ":" << time.wHour << ":"
					<< time.wMinute << time.wSecond << endl;
				cout << "��Ϣ���ͳɹ�" << endl;
				cout << "-----------------------------" << endl;
			}
			
		}
	}
}
int main()
{
	int isError;
	WSADATA wsaData;
	SOCKET ClientSocket = INVALID_SOCKET;

	//��ʼ��Winsock
	isError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (isError != NO_ERROR)
	{
		cout << "��ʼ��WSAʧ��" << endl;
		return 0;
	}

	//�ͻ��˴���Socket
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET) {
		cout << "�ͻ���Socket����ʧ��" << endl;
		WSACleanup();
		return 0;
	}

	//Ҫ���ӷ�������IP��ַ���˿ں�
	struct sockaddr_in Server;
	Server.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &Server.sin_addr.s_addr);
	Server.sin_port = htons(CONNECT_PORT);

	//���ӷ����
	isError = connect(ClientSocket, (SOCKADDR*)&Server, sizeof(Server));
	if (isError == SOCKET_ERROR) {
		cout << "���ӷ����ʧ��"<< endl;
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	recv(ClientSocket, user_name, 20, 0);

	// ��ӡ��������ı�־
	cout << "              Welcome    User    " << user_name << endl;
	cout << "*****************************************************" << endl;
	cout << "             Use quit command to quit" << endl;
	cout << "-----------------------------------------------------" << endl;

	//----------------------
	// ���������̣߳�һ�������̣߳�һ�������߳�
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, Recv, (LPVOID)&ClientSocket, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, Send, (LPVOID)&ClientSocket, 0, NULL);

	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);

	// �ر�socket
	closesocket(ClientSocket);
	WSACleanup();
	return 0;


	return 0;
}