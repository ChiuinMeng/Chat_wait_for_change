﻿#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
	#include <winsock2.h>
	#include <Windows.h>
	#include <inaddr.h>
	#pragma comment(lib, "ws2_32")
	typedef int socklen_t;
#else
	#include<unistd.h> //uni std
	#include<arpa/inet.h>
	typedef unsigned int SOCKET;
	#define INVALID_SOCKET  (SOCKET)(~0)
	#define SOCKET_ERROR            (-1)
	typedef unsigned int socklen_t;
#endif

#include <iostream>
#include <vector>
#include <cstring>

#include "DataPacket.h"

std::vector<SOCKET> g_clients;

int  processor(SOCKET _cSock);

int main()
{
	unsigned short port = 10086;

#ifdef _WIN32
	WSADATA wsaData;
	int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (err != 0)  return -1;
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		WSACleanup();
		printf("version failed");
		return -1;
	}
	else {
		printf("version success");
	}
#endif

	// 2.创建一个socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


	// 3.创建协议地址族
	sockaddr_in addrSrv = { 0 };
	addrSrv.sin_family = AF_INET;
#ifdef _WIN32
	// 如果是服务端，就别用127.0.0.1，因为这是回环地址。
	addrSrv.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	addrSrv.sin_addr.s_addr = INADDR_ANY;
#endif
	addrSrv.sin_port = htons(port);//端口号


	if (SOCKET_ERROR == bind(serverSocket, (sockaddr*)& addrSrv, sizeof(sockaddr))) {
		printf("erro, bind port(%d)(%d) failed\n",port, addrSrv.sin_port);
	}
	else {
		printf("bind port(%d)(%d) success\n",port, addrSrv.sin_port);
	}


	// 5.监听
	if (SOCKET_ERROR == listen(serverSocket, 10)) { //监听数：10
		printf("listen port(%d)(%d) failed\n", port, addrSrv.sin_port);
	}
	else {
		printf("listen port(%d)(%d) success\n",port, addrSrv.sin_port);
	}

	// 6.通信
	while (true)   //循环处理socket消息
	{
		// 伯克利 BSD socket
		// fd_set最多可管理64个套接字
		fd_set fdRead;  //描述符(socket)集合
		fd_set fdWrite;
		fd_set fdExp;
		// FD_ZERO函数将集合初始化为空集合
		FD_ZERO(&fdRead);
		FD_ZERO(&fdWrite);
		FD_ZERO(&fdExp);

		/* 
			赋值，此步必须做。
			select函数返回后，会修改每个fd_set结构，删除那些不存在的没有完成I/O操作的套接字。
			如果你不赋值，我删啥？我改啥？
			在此步，可以有选择地赋值，也就是只把自己“感兴趣”的套接字加入集合。
		 */
		// FD_SET函数将套接字s加入set集合
		FD_SET(serverSocket, &fdRead);
		FD_SET(serverSocket, &fdWrite);
		FD_SET(serverSocket, &fdExp);
		for (unsigned int n = 0; n < g_clients.size(); n++) {
			FD_SET(g_clients[n], &fdRead);
		}

		//select第一个参数nfds是一个整数值，是指fd_set集合中的所有描述符（socket）的范围，而不是大小。
		//既是所有文件描述符的最大值+1，在Windows中这个参数可以为0（因为宏的存在）
		//timeout参数若为NULL，select会阻塞!
		//timeout设置为0，则不会阻塞，直接返回。
		//timeout表示的是最大的一个时间值， 实际上并不一定会阻塞那么久。
		// 通过该函数，可以判断套接字是否存在数据，或者能否向其写入数据。
		// 
		//timeval t = { 0,0 };
		// select函数返回处于就绪状态并且已经包含在fd_set结构中的套接字总数
		//int s_r = select(serverSocket + 1, &fdRead, &fdWrite, &fdExp, &t);
		// 
		// 如果没有，就算你前面再怎么FD_SET使得fd_count值变大，这里也会被置为0。

		/*	readfds参数将包含符合下面任何一个条件的套接字：
			1. 假如已经调用listen()函数，并且一个连接正在建立。那么此时调用accept()函数会成功。
			   fd_count变为1，fd_array[0]变为服务器的socket值（因为此时还没有调用accept()函数获取客户端的socket值，连接尚未建立。）
			2. 有数据可以读入。此时在该套接字上调用recv等输入函数，立即接受到对方的数据(非阻塞)。
			   fd_count变为1，fd_array[0]变为客户端的socket值（因为连接已经建立了）。
			3. 连接已经关闭、重设或中断。
			如果一个都没有，就算你前面再怎么调用FD_SET函数使得fd_count值变大，这里也会被置为0。
		*/
		int s_r = select(serverSocket + 1, &fdRead, &fdWrite, &fdExp, NULL);
		if (s_r < 0) {
			printf("select finish\n");
			break;
		}
		else if (s_r == 0) {
			/*
			if (t.tv_sec == 0 && t.tv_usec == 0) {
				// printf("无socket事件\n");
			}
			else {
				printf("select函数调用超时\n");
			}
			*/
		}
		else {
			printf("有socket事件发生\n");
			// TODO: 判断是什么事件发生了
		}


		// 如果服务器本机的socket仍然在里面，说明有新的客户端请求连接，下面就要调用accept()函数完成连接了。
		// FD_ISSET函数判断s是否是set集合的一名成员，如果是，返回true。
		if (FD_ISSET(serverSocket, &fdRead)) {   
			// FD_CLR函数将套接字s从set集合中删除。
			FD_CLR(serverSocket, &fdRead);
			sockaddr_in addr = { 0 };  //存客户端的网络地址
			socklen_t addrlen = sizeof(sockaddr_in);
			SOCKET clientSocket = INVALID_SOCKET;
			clientSocket = accept(serverSocket, (sockaddr*)& addr, &addrlen);//因为select了，所以不会在这里阻塞
			if (INVALID_SOCKET == clientSocket) {
				printf("accept failed\n");
			}
			else {
				g_clients.push_back(clientSocket);     //将客户端socket添加进动态数组
				printf("accept success, new client: socket = %d, ip = %s\n", clientSocket, inet_ntoa(addr.sin_addr));
			}
		}


		for (unsigned int i = 0; i < g_clients.size(); i++) {
			if (FD_ISSET(g_clients[i], &fdRead)) {    //该套接字可读
				if (-1 == processor(g_clients[i])) {
					auto iter = g_clients.begin();
					if (iter != g_clients.end()) {
						g_clients.erase(iter);
					}
				}
			}
			if (FD_ISSET(g_clients[i], &fdWrite) ){    //该套接字可写
				if (-1 == processor(g_clients[i])) {
					auto iter = g_clients.begin();
					if (iter != g_clients.end()) {
						g_clients.erase(iter);
					}
				}
			}
		}
		/*
		for (unsigned int n = 0; n < fdRead.fd_count; n++) {
			if (-1 == processor(fdRead.fd_array[n])) {  //处理客户端请求
				auto iter = find(g_clients.begin(), g_clients.end(), fdRead.fd_array[n]);
				if (iter != g_clients.end()) {
					g_clients.erase(iter);
				}
			}
		}
		*/
		// TODO: 服务器做其它事情，即便没有客户端事件发生。
		// 空闲时间，处理其它业务
	}

#ifdef _WIN32
	for (int n = g_clients.size() - 1; n >= 0; n--) {
		closesocket(g_clients[n]);
	}
	closesocket(serverSocket);
	WSACleanup();
#else
	for (int n = g_clients.size() - 1; n >= 0; n--) {
		close(g_clients[n]);
	}
	close(serverSocket);
#endif

	getchar(); //防止程序一闪而过
	return	0;
}

/*
    处理客户端消息
*/
int  processor(SOCKET _cSock)
{
	char szRecv[4096] = {};// 缓冲区

	// 接受客户端数据
	int nLen = recv(_cSock, szRecv, sizeof(DataHeader), 0); //先接受数据头DataHeader
	if (nLen <= 0) {
		printf("client<%d> exit\n", _cSock);
		return -1; //该客户端已断开连接
	}

	DataHeader* header = (DataHeader*)szRecv;
	recv(_cSock, szRecv + sizeof(DataHeader), header->dataLength - sizeof(DataHeader), 0); //再收取后半段数据体

	// 根据数据头中的命令，选择不同分支，处理不同请求。
	switch (header->cmd) {
	case CMD_LOGIN: {
		Login* login = (Login*)szRecv;
		printf("receive command from client<%d>: CMD_LOGIN, data length: %d, userName=%s\n", _cSock, login->dataLength, login->userName);
		//忽略判断用户密码是否正确的过程
		LoginResult ret;
		ret.hostID = _cSock;
		send(_cSock, (char*)& ret, sizeof(LoginResult), 0);
		break;
	}
	case CMD_ENTER_GROUP_CHAT: {
		printf("client<%d>:CMD_ENTER_GROUP_CHAT ，dLen: %d\n",_cSock,header->dataLength);
		EnterGroupChat* egc = (EnterGroupChat*)szRecv;
		printf("%s 加入群聊\n", egc->userName);

		BroadcastMessage bm;
		char bMessage[256];
		memset(bMessage, '\0', sizeof(bMessage));
		strcat(bMessage, egc->userName);
		strcat(bMessage, "加入群聊");
		strcpy(bm.bMessage,bMessage);
		strcpy(bm.userName, egc->userName);
		bm.userID = _cSock;
		for (unsigned int i = 0; i < g_clients.size(); i++) {
			if (g_clients[i] == _cSock)continue;
			send(g_clients[i], (char*)& bm, sizeof(BroadcastMessage), 0);//?
		}

		EnterGroupChatResult egcr;
		egcr.result = 1;
		if (SOCKET_ERROR == send(_cSock, (char*)& egcr, sizeof(egcr), 0)) {
			printf("发送失败\n");
		}
		else {
			printf("发送成功\n");
		}
		break;
	}
	case CMD_EXIT_GROUP_CHAT: {
		printf("client<%d>:CMD_EXIT_GROUP_CHAT ，dLen: %d\n", _cSock, header->dataLength);
		ExitGroupChat* egc = (ExitGroupChat*)szRecv;
		printf("%s 退出群聊\n", egc->userName);

		BroadcastMessage bm;
		char bMessage[256] = {};
		strcat(bMessage, egc->userName);
		strcat(bMessage, "退出群聊");
		strcpy(bm.bMessage, bMessage);
		strcpy(bm.userName, egc->userName);
		bm.userID = _cSock;
		for (unsigned int i = 0; i < g_clients.size(); i++) {
			if (g_clients[i] == _cSock)continue;
			send(g_clients[i], (char*)&bm, sizeof(BroadcastMessage), 0);
		}

		ExitGroupChatResult egcr;
		send(_cSock, (char*)& egcr, sizeof(egcr), 0);
		break;
	}
	case CMD_BROADCAST_MESSAGE: {
		printf("client<%d>:CMD_BROADCAST_MESSAGE ，dLen: %d\n", _cSock, header->dataLength);
		BroadcastMessage* bm = (BroadcastMessage*)szRecv;
		printf("用户:%s 发送群聊消息:%s", bm->userName, bm->bMessage);
		bm->userID = _cSock;

		for (unsigned int i = 0; i < g_clients.size(); i++) {
			if (g_clients[i] == _cSock)continue;
			send(g_clients[i], szRecv, sizeof(BroadcastMessage), 0);
		}

		BroadcastMessageResult bmr;
		send(_cSock, (char*)& bmr, sizeof(bmr), 0);
		break;
	}
	case CMD_CHAT_WITH_USER: {
		printf("client<%d>:CMD_CHAT_WITH_USER ，dLen: %d\n", _cSock, header->dataLength);
		ChatWithUser* cwu = (ChatWithUser*)szRecv;

		unsigned int i = 0;
		for (; i < g_clients.size(); i++) {
			if (g_clients[i] == _cSock)continue;
			if (g_clients[i] == cwu->receiverID)break;
		}

		ChatWithUserResult cwur;
		if (i == g_clients.size()) cwur.receiverStatus = 0;
		else cwur.receiverStatus = 1;
		send(_cSock, (char*)& cwur, sizeof(cwur), 0);
		break;
	}
	case CMD_USER_MESSAGE: {
		printf("client<%d>:CMD_USER_MESSAGE ，dLen: %d\n", _cSock, header->dataLength);
		UserMessage* um = (UserMessage*)szRecv;
		um->senderID = _cSock;

		unsigned int i = 0;
		for (; i < g_clients.size(); i++) {
			if (g_clients[i] == um->receiverID) {
				send(g_clients[i], szRecv, sizeof(UserMessage), 0);
				break;
			}
		}
		if (i == g_clients.size()) {
			ChatWithUserResult cwur;
			cwur.receiverStatus = 0;
			send(_cSock, (char*)& cwur, sizeof(cwur), 0);
		}
		break;
	}
	default:
		printf("client<%d>: 未知命令, dLen: %d\n", _cSock, header->dataLength);
		DataHeader h;
		h.cmd = CMD_ERROR;
		h.dataLength = sizeof(DataHeader);
		send(_cSock, (char*)& h, sizeof(h), 0);
		break;
	}
	return 0;
}