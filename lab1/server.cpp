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
map<SOCKET, int> client_map;//如果value=1，说明clinet在线

DWORD WINAPI handlerRequest(LPVOID lparam)
{
	SOCKET ClientSocket = (SOCKET)(LPVOID)lparam;
	client_map[ClientSocket] = 1;//表示client在线

	char user_name[20];
	strcpy_s(user_name, to_string(ClientSocket).data());//将ClientSocket的信息作为名字
	send(ClientSocket, user_name, 20, 0);//发送客户端名字给客户端

	SYSTEMTIME time;
	GetLocalTime(&time);
	cout << time.wYear << ":" << time.wMonth << ":"
		<< time.wDay << ":" << time.wHour << ":"
		<< time.wMinute << time.wSecond << endl;

	cout << "用户：" << user_name << "加入聊天" << endl;
	cout << "-----------------------------" << endl;

	int isRecv, isSend;
	int flag = 1;//该客户端是否退出
	int toall = 1;//是否群发，1为群发
	do {
		char recvMessage[MESSAGE_LEN];
		char sendMessage[MESSAGE_LEN];
		char speMessage[MESSAGE_LEN];
		isRecv = recv(ClientSocket, recvMessage, MESSAGE_LEN, 0);
		toall = 1;//????
		if (isRecv > 0)
		{
			char special_user_name[20] = "";//私聊的用户名
			for (int i = 0; i < MESSAGE_LEN; i++) {
				if (recvMessage[i] == '|') { //发现"|"即说明为私聊
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

			strcpy_s(sendMessage, "用户:");
			string ClientID = to_string(ClientSocket);
			strcat_s(sendMessage, ClientID.data()); // data函数直接转换为char*
			strcat_s(sendMessage, "--: ");
			strcat_s(sendMessage, recvMessage);

			SYSTEMTIME time;
			GetLocalTime(&time);
			cout << time.wYear << ":" << time.wMonth << ":"
				<< time.wDay << ":" << time.wHour << ":"
				<< time.wMinute << time.wSecond << endl;
			cout << "用户：" << ClientSocket << "的消息:" << recvMessage << endl;
			cout << "-----------------------------" << endl;

			if (toall == 1) {
				for (auto it : client_map) {
					if (it.first != ClientSocket && it.second == 1) {
						isSend = send(it.first, sendMessage, MESSAGE_LEN, 0);
						if (isSend == SOCKET_ERROR)
							cout << "发送失败" << endl;
					}
				}
			}
			else {
				for (auto it : client_map) {
					if (to_string(it.first) == string(special_user_name) && it.second == 1) {
						isSend = send(it.first, speMessage, MESSAGE_LEN, 0);
						if (isSend == SOCKET_ERROR)
							cout << "发送失败" << endl;
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
	cout << "用户：" << ClientSocket << "离开了聊天室" << endl;

	closesocket(ClientSocket);
	return 0;
}
int main()
{
	int isError;
	WSADATA wsaData;
	SOCKET ClientSocket = INVALID_SOCKET;

	//初始化Winsock
	isError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (isError != NO_ERROR)
	{
		cout << "初始化WSA失败" << endl;
		return 0;
	}

	// 创建一个监听的SOCKET
	// 如果有connect的请求就新创建一个线程
	SOCKET ListenSocket;
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		cout << "监听Socket创建失败"<< endl;
		WSACleanup();
		return 0;
	}

	//服务器端IP和端口设置
	sockaddr_in Server;
	Server.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &Server.sin_addr.s_addr);
	Server.sin_port = htons(Server_PORT);
	isError = bind(ListenSocket, (SOCKADDR*)&Server, sizeof(Server));
	if (isError == SOCKET_ERROR) {
		cout << "服务器端绑定ip和端口失败" << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// 监听即将到来的请求信号
	if (listen(ListenSocket, 5) == SOCKET_ERROR) {
		cout << "监听失败" << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	//使用多线程处理多个请求
	while (1) {
		sockaddr_in Client_addr;
		int len = sizeof(sockaddr_in);
		
		SOCKET AcceptSocket = accept(ListenSocket, (SOCKADDR*)&Client_addr, &len);
		if (AcceptSocket == INVALID_SOCKET) {
			cout << "接收客户端Socket失败" << endl;
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}
		else {
			// 创建线程，并且传入与client通讯的套接字
			HANDLE hThread = CreateThread(NULL, 0, handlerRequest, (LPVOID)AcceptSocket, 0, NULL);
			CloseHandle(hThread); // 关闭对线程的引用
		}
	}

	// 关闭服务端SOCKET
	closesocket(ListenSocket);

	WSACleanup();
	return 0;
}