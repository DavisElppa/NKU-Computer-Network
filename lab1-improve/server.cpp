#include<iostream>
#include<string>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<map>
#include <typeinfo>
#pragma comment(lib,"ws2_32.lib")

#define MESSAGE_LEN 256
#define Server_PORT 33414

using namespace std;
map<SOCKET, int> client_map;//���value=1��˵��clinet����
map<string, SOCKET>client_name; //����string��SOCKET��ӳ�䣬�����û�������ת����Ϣ

//�г���ǰ�����û�
void listAll(char* user_name)
{
	char sendMessage[MESSAGE_LEN];
	strcpy_s(sendMessage, "��ǰ�����û���");

	//�ҳ����������û�
	for (auto it : client_name)
	{
		if (client_map[it.second] == 1)//˵�����û�����
		{
			strcat_s(sendMessage, it.first.data());
			strcat_s(sendMessage, " ");
		}
	}

	send(client_name[user_name], sendMessage, MESSAGE_LEN, 0);
}

//����Ϣ���͸������û�
void sendToall(char* send_user,char* msg)//msgΪrecvMessage
{
	char sendMessage[MESSAGE_LEN];
	strcpy_s(sendMessage, "�û�");
	strcat_s(sendMessage, send_user); // data����ֱ��ת��Ϊchar*
	strcat_s(sendMessage, ": ");
	strcat_s(sendMessage, msg);

	for (auto it : client_name)
	{
		if (it.first != string(send_user) && client_map[it.second] == 1)
		{
			int isSend = send(it.second, sendMessage, MESSAGE_LEN, 0);
			if (isSend == SOCKET_ERROR)
			{
				SYSTEMTIME time;
				GetLocalTime(&time);
				cout << time.wYear << ":" << time.wMonth << ":"
					<< time.wDay << ":" << time.wHour << ":"
					<< time.wMinute << ":" << time.wSecond << endl;
				cout << "��־--�û�" << send_user << "���û�" << it.second << "������Ϣʧ��" << endl;
				cout << "-----------------------------" << endl;
			}
		}
	}
}

//˽����Ϣ
void sendTotarget(char* send_user, char* msg)//msgΪrecvMessage
{
	char target_user[20];
	char sendMessage[MESSAGE_LEN];
	for (int j = 1; j < MESSAGE_LEN; j++) {
		if (msg[j] == ':') { // ":"��Ϊ���͵���Ϣ
			target_user[j - 1] = '\0';
			for (int z = j + 1; z < MESSAGE_LEN; z++) {
				sendMessage[z - j - 1] = msg[z];
			}
			strcat_s(sendMessage, "(����");
			strcat_s(sendMessage, send_user);
			strcat_s(sendMessage, "��˽����Ϣ)\0");
			break;
		}
		else {
			target_user[j - 1] = msg[j];
		}
	}
	if (client_name.find(string(target_user)) == client_name.end()||client_map[client_name[string(target_user)]]==0)
	{
		SYSTEMTIME time;
		GetLocalTime(&time);
		cout << time.wYear << ":" << time.wMonth << ":"
			<< time.wDay << ":" << time.wHour << ":"
			<< time.wMinute << ":" << time.wSecond << endl;
		cout << "��־--�û�"<<send_user<<"˽���û�"<< target_user<<"ʧ�ܣ���Ϊ���û������ڻ���������"<< endl;
		cout << "-----------------------------" << endl;

		//��Ҫ����Ϣ���͸�send_user
		strcpy_s(sendMessage, "˽��ʧ�ܣ��û�");
		strcat_s(sendMessage, target_user);
		strcat_s(sendMessage, "�����ڻ���������");

		int isSend = send(client_name[string(send_user)], sendMessage, MESSAGE_LEN, 0);
		if (isSend == SOCKET_ERROR)
		{
			GetLocalTime(&time);
			cout << time.wYear << ":" << time.wMonth << ":"
				<< time.wDay << ":" << time.wHour << ":"
				<< time.wMinute << ":" << time.wSecond << endl;
			cout << "��־--˽��ʧ����Ϣ�������û�" << send_user << "ʧ��" << endl;
			cout << "-----------------------------" << endl;
		}		
	}
	else
	{
		SOCKET target = client_name[string(target_user)];
		if (client_map[target] == 1)
		{
			int isSend = send(target, sendMessage, MESSAGE_LEN, 0);
			if (isSend == SOCKET_ERROR)
			{
				SYSTEMTIME time;
				GetLocalTime(&time);
				cout << time.wYear << ":" << time.wMonth << ":"
					<< time.wDay << ":" << time.wHour << ":"
					<< time.wMinute << ":" << time.wSecond << endl;
				cout << "��־--�û�" << send_user << "���û�" << target_user << "������Ϣʧ��" << endl;
				cout << "-----------------------------" << endl;
			}			
		}
	}
}

DWORD WINAPI handlerRequest(LPVOID lparam)
{
	SOCKET ClientSocket = (SOCKET)(LPVOID)lparam;
	client_map[ClientSocket] = 1;//��ʾclient����

	char user_name[20];//���ڽ����û���

	recv(ClientSocket, user_name, 20, 0);//�������Կͻ��˵��û���

	//�����ֺ�SOCKET��
	client_name[string(user_name)] = ClientSocket;

	SYSTEMTIME time;
	GetLocalTime(&time);
	cout << time.wYear << ":" << time.wMonth << ":"
		<< time.wDay << ":" << time.wHour << ":"
		<< time.wMinute << ":" << time.wSecond << endl;

	cout << "��־--�û���" << user_name << "��������" << endl;
	cout << "-----------------------------" << endl;

	int isRecv, isSend;
	int flag = 1;//�ÿͻ����Ƿ��˳�
	do {
		char recvMessage[MESSAGE_LEN];
		char sendMessage[MESSAGE_LEN];
		char speMessage[MESSAGE_LEN];
		isRecv = recv(ClientSocket, recvMessage, MESSAGE_LEN, 0);
		
		if (isRecv > 0)
		{
			GetLocalTime(&time);
			cout << time.wYear << ":" << time.wMonth << ":"
				<< time.wDay << ":" << time.wHour << ":"
				<< time.wMinute << ":" << time.wSecond << endl;
			cout << "��־--�û�" << user_name << "����Ϣ:" << recvMessage << endl;
			cout << "-----------------------------" << endl;
			/*
			* �Լ�Ҫ�����޸��Ǽ�⵽"@"��˽��
			* ����˽�ģ�����Ϣ�����͸�@����û�
			* �����Ⱥ�������͸������û�
			* �鿴��ǰ�����û���ʹ��lsָ��
			*/
			if (string(recvMessage) == "ls") //�鿴��ǰ���������û�
			{
				listAll(user_name);
			}
			else if (recvMessage[0] == '@')
			{
				sendTotarget(user_name, recvMessage);
			}
			else
			{
				sendToall(user_name, recvMessage);
			}
		}
		else
		{
			flag = 0;//�˳���Ⱥ��
			client_map[ClientSocket] = 0;
		}
	} while (isRecv != SOCKET_ERROR && flag != 0);

	GetLocalTime(&time);
	cout << time.wYear << ":" << time.wMonth << ":"
		<< time.wDay << ":" << time.wHour << ":"
		<< time.wMinute <<":"<< time.wSecond << endl;
	cout << "��־--�û���" << user_name << "�뿪��������" << endl;
	cout << "-----------------------------" << endl;

	closesocket(ClientSocket);
	return 0;
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

	// ����һ��������SOCKET
	// �����connect��������´���һ���߳�
	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "����Socket����ʧ��"<< endl;
		WSACleanup();
		return 0;
	}

	//��������IP�Ͷ˿�����
	sockaddr_in Server;
	Server.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &Server.sin_addr.s_addr);
	Server.sin_port = htons(Server_PORT);
	isError = bind(ListenSocket, (SOCKADDR*)&Server, sizeof(Server));
	if (isError == SOCKET_ERROR) {
		cout << "�������˰�ip�Ͷ˿�ʧ��" << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// �������������������ź�
	if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		cout << "����ʧ��" << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//ʹ�ö��̴߳���������
	while (1) {
		sockaddr_in Client_addr;
		int len = sizeof(sockaddr_in);
		
		SOCKET AcceptSocket = accept(ListenSocket, (SOCKADDR*)&Client_addr, &len);
		if (AcceptSocket == INVALID_SOCKET) {
			cout << "���տͻ���Socketʧ��" << endl;
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		else {
			// �����̣߳����Ҵ�����clientͨѶ���׽���
			HANDLE hThread = CreateThread(NULL, 0, handlerRequest, (LPVOID)AcceptSocket, 0, NULL);
			CloseHandle(hThread); // �رն��̵߳�����
		}
	}

	// �رշ����SOCKET
	closesocket(ListenSocket);

	WSACleanup();
	return 0;
}