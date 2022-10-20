/*
* 预计实现的附加功能
* 多人聊天室--通过服务端转发
* 多线程
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

int flag = 1;//判断是否退出
char user_name[20];

DWORD WINAPI Recv(LPVOID lparam_socket) {
	int isRecv;//判断是否接收到消息
	SOCKET* recvSocket = (SOCKET*)lparam_socket;//使用指针的原因为，需要指向连接socket的地址
	while (1)
	{
		char recvMessage[MESSAGE_LEN];
		isRecv = recv(*recvSocket, recvMessage, MESSAGE_LEN, 0);
		if (isRecv > 0 && flag == 1)//接收到消息，且没有退出群聊，因为是双线程，所以需要判断flag
		{
			SYSTEMTIME time;
			GetLocalTime(&time);
			cout << time.wYear << ":" << time.wMonth << ":"
				<< time.wDay << ":" << time.wHour << ":"
				<< time.wMinute << time.wSecond << endl;
			cout << "接收到消息:" << recvMessage << endl;
			cout << "-----------------------------" << endl;
			cout << "请输入你要发送的消息" << endl;
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
	SOCKET* sendSocket = (SOCKET*)lparam_socket;//使用指针的原因为，需要指向连接socket的地址

	while (1)
	{
		cout << "请输入你要发送的消息" << endl;
		char sendMessage[MESSAGE_LEN];
		cin.getline(sendMessage, MESSAGE_LEN);//getline可以识别空格，遇到换行自动结束

		if (string(sendMessage) == "quit")//表示该用户要退出群聊
		{
			flag = 0;
			closesocket(*sendSocket);
			WSACleanup();//调用WSACleanup函数来解除与Socket库的绑定并且释放Socket库所占用的系统资源
			return 0;
		}
		else
		{
			isSend = send(*sendSocket, sendMessage, MESSAGE_LEN, 0);
			if (isSend == SOCKET_ERROR)//-1表示出错
			{
				cout << "发送信息失败:" << WSAGetLastError()<<endl;
				closesocket(*sendSocket);
				WSACleanup();//调用WSACleanup函数来解除与Socket库的绑定并且释放Socket库所占用的系统资源
				return 0;
			}
			else
			{
				SYSTEMTIME time;
				GetLocalTime(&time);
				cout << time.wYear << ":" << time.wMonth << ":"
					<< time.wDay << ":" << time.wHour << ":"
					<< time.wMinute << time.wSecond << endl;
				cout << "消息发送成功" << endl;
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

	//初始化Winsock
	isError = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (isError != NO_ERROR)
	{
		cout << "初始化WSA失败" << endl;
		return 0;
	}

	//客户端创建Socket
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET) {
		cout << "客户端Socket创建失败" << endl;
		WSACleanup();
		return 0;
	}

	//要连接服务器的IP地址，端口号
	struct sockaddr_in Server;
	Server.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &Server.sin_addr.s_addr);
	Server.sin_port = htons(CONNECT_PORT);

	//连接服务端
	isError = connect(ClientSocket, (SOCKADDR*)&Server, sizeof(Server));
	if (isError == SOCKET_ERROR) {
		cout << "连接服务端失败"<< endl;
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	recv(ClientSocket, user_name, 20, 0);

	// 打印进入聊天的标志
	cout << "              Welcome    User    " << user_name << endl;
	cout << "*****************************************************" << endl;
	cout << "             Use quit command to quit" << endl;
	cout << "-----------------------------------------------------" << endl;

	//----------------------
	// 创建两个线程，一个接受线程，一个发送线程
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, Recv, (LPVOID)&ClientSocket, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, Send, (LPVOID)&ClientSocket, 0, NULL);

	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);
	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);

	// 关闭socket
	closesocket(ClientSocket);
	WSACleanup();
	return 0;


	return 0;
}