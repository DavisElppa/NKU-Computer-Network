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

DWORD WINAPI handlerRequest(LPVOID lparam)
{
	SOCKET ClientSocket = (SOCKET)(LPVOID)lparam;
	client_map[ClientSocket] = 1;//��ʾclient����

	char user_name[20];
	strcpy_s(user_name, to_string(ClientSocket).data());//��ClientSocket����Ϣ��Ϊ����
	send(ClientSocket, user_name, 20, 0);//���Ϳͻ������ָ��ͻ���

	SYSTEMTIME time;
	GetLocalTime(&time);
	cout << time.wYear << ":" << time.wMonth << ":"
		<< time.wDay << ":" << time.wHour << ":"
		<< time.wMinute << time.wSecond << endl;

	cout << "�û���" << user_name << "��������" << endl;
	cout << "-----------------------------" << endl;

	int isRecv, isSend;
	int flag = 1;//�ÿͻ����Ƿ��˳�
	int toall = 1;//�Ƿ�Ⱥ����1ΪȺ��
	do {
		char recvMessage[MESSAGE_LEN];
		char sendMessage[MESSAGE_LEN];
		char speMessage[MESSAGE_LEN];
		isRecv = recv(ClientSocket, recvMessage, MESSAGE_LEN, 0);
		toall = 1;//????
		if (isRecv > 0)
		{
			char special_user_name[20] = "";//˽�ĵ��û���
			for (int i = 0; i < MESSAGE_LEN; i++) {
				if (recvMessage[i] == '|') { //����"|"��˵��Ϊ˽��
					toall = 0;
				}
			}

			if (toall == 0) {
				int j;
				for (j = 0; j < MESSAGE_LEN; j++) {
					if (recvMessage[j] == '|') {
						special_user_name[j] = '\0';
						for (int z = j + 1; z < MESSAGE_LEN; z++) {
							speMessage[z - j - 1] = recvMessage[z];
						}
						strcat_s(speMessage, "( from ");
						strcat_s(speMessage, to_string(ClientSocket).data());
						strcat_s(speMessage, ")\0");
						break;
					}
					else {
						special_user_name[j] = recvMessage[j];
					}
				}
			}

			strcpy_s(sendMessage, "�û�:");
			string ClientID = to_string(ClientSocket);
			strcat_s(sendMessage, ClientID.data()); // data����ֱ��ת��Ϊchar*
			strcat_s(sendMessage, "--: ");
			strcat_s(sendMessage, recvMessage);

			SYSTEMTIME time;
			GetLocalTime(&time);
			cout << time.wYear << ":" << time.wMonth << ":"
				<< time.wDay << ":" << time.wHour << ":"
				<< time.wMinute << time.wSecond << endl;
			cout << "�û���" << ClientSocket << "����Ϣ:" << recvMessage << endl;
			cout << "-----------------------------" << endl;

			if (toall == 1) {
				for (auto it : client_map) {
					if (it.first != ClientSocket && it.second == 1) {
						isSend = send(it.first, sendMessage, MESSAGE_LEN, 0);
						if (isSend == SOCKET_ERROR)
							cout << "����ʧ��" << endl;
					}
				}
			}
			else {
				for (auto it : client_map) {
					if (to_string(it.first) == string(special_user_name) && it.second == 1) {
						isSend = send(it.first, speMessage, MESSAGE_LEN, 0);
						if (isSend == SOCKET_ERROR)
							cout << "����ʧ��" << endl;
					}
				}
			}
		}
		else
		{
			flag = 0;
		}
	} while (isRecv != SOCKET_ERROR && flag != 0);

	GetLocalTime(&time);
	cout << time.wYear << ":" << time.wMonth << ":"
		<< time.wDay << ":" << time.wHour << ":"
		<< time.wMinute << time.wSecond << endl;
	cout << "�û���" << ClientSocket << "�뿪��������" << endl;

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