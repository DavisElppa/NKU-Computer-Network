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
map<string, SOCKET>client_name; //创建string到SOCKET的映射，根据用户名进行转发消息

//列出当前所有用户
void listAll(char* user_name)
{
	char sendMessage[MESSAGE_LEN];
	strcpy_s(sendMessage, "当前在线用户：");

	//找出所有在线用户
	for (auto it : client_name)
	{
		if (client_map[it.second] == 1)//说明该用户在线
		{
			strcat_s(sendMessage, it.first.data());
			strcat_s(sendMessage, " ");
		}
	}

	send(client_name[user_name], sendMessage, MESSAGE_LEN, 0);
}

//将信息发送给所有用户
void sendToall(char* send_user,char* msg)//msg为recvMessage
{
	char sendMessage[MESSAGE_LEN];
	strcpy_s(sendMessage, "用户");
	strcat_s(sendMessage, send_user); // data函数直接转换为char*
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
				cout << "日志--用户" << send_user << "向用户" << it.second << "发送信息失败" << endl;
				cout << "-----------------------------" << endl;
			}
		}
	}
}

//私聊信息
void sendTotarget(char* send_user, char* msg)//msg为recvMessage
{
	char target_user[20];
	char sendMessage[MESSAGE_LEN];
	for (int j = 1; j < MESSAGE_LEN; j++) {
		if (msg[j] == ':') { // ":"后为发送的消息
			target_user[j - 1] = '\0';
			for (int z = j + 1; z < MESSAGE_LEN; z++) {
				sendMessage[z - j - 1] = msg[z];
			}
			strcat_s(sendMessage, "(来自");
			strcat_s(sendMessage, send_user);
			strcat_s(sendMessage, "的私聊信息)\0");
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
		cout << "日志--用户"<<send_user<<"私聊用户"<< target_user<<"失败，因为该用户不存在或者已下线"<< endl;
		cout << "-----------------------------" << endl;

		//还要把信息发送给send_user
		strcpy_s(sendMessage, "私聊失败：用户");
		strcat_s(sendMessage, target_user);
		strcat_s(sendMessage, "不存在或者已下线");

		int isSend = send(client_name[string(send_user)], sendMessage, MESSAGE_LEN, 0);
		if (isSend == SOCKET_ERROR)
		{
			GetLocalTime(&time);
			cout << time.wYear << ":" << time.wMonth << ":"
				<< time.wDay << ":" << time.wHour << ":"
				<< time.wMinute << ":" << time.wSecond << endl;
			cout << "日志--私聊失败信息反馈给用户" << send_user << "失败" << endl;
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
				cout << "日志--用户" << send_user << "向用户" << target_user << "发送信息失败" << endl;
				cout << "-----------------------------" << endl;
			}			
		}
	}
}

DWORD WINAPI handlerRequest(LPVOID lparam)
{
	SOCKET ClientSocket = (SOCKET)(LPVOID)lparam;
	client_map[ClientSocket] = 1;//表示client在线

	char user_name[20];//用于接收用户名

	recv(ClientSocket, user_name, 20, 0);//接收来自客户端的用户名

	//把名字和SOCKET绑定
	client_name[string(user_name)] = ClientSocket;

	SYSTEMTIME time;
	GetLocalTime(&time);
	cout << time.wYear << ":" << time.wMonth << ":"
		<< time.wDay << ":" << time.wHour << ":"
		<< time.wMinute << ":" << time.wSecond << endl;

	cout << "日志--用户：" << user_name << "加入聊天" << endl;
	cout << "-----------------------------" << endl;

	int isRecv, isSend;
	int flag = 1;//该客户端是否退出
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
			cout << "日志--用户" << user_name << "的消息:" << recvMessage << endl;
			cout << "-----------------------------" << endl;
			/*
			* 自己要做的修改是检测到"@"是私聊
			* 发现私聊，则信息仅发送给@后的用户
			* 如果是群发，则发送给所有用户
			* 查看当前所有用户，使用ls指令
			*/
			if (string(recvMessage) == "ls") //查看当前在线所有用户
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
			flag = 0;//退出了群聊
			client_map[ClientSocket] = 0;
		}
	} while (isRecv != SOCKET_ERROR && flag != 0);

	GetLocalTime(&time);
	cout << time.wYear << ":" << time.wMonth << ":"
		<< time.wDay << ":" << time.wHour << ":"
		<< time.wMinute <<":"<< time.wSecond << endl;
	cout << "日志--用户：" << user_name << "离开了聊天室" << endl;
	cout << "-----------------------------" << endl;

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