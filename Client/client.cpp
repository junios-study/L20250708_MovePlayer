#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32")

unsigned RecvThread(void* Arg)
{
	SOCKET ServerSocket = *(SOCKET*)Arg;

	while (true)
	{
		//recv()
	}

	return 0;
}

unsigned SendThread(void* Arg)
{
	SOCKET ServerSocket = *(SOCKET*)Arg;

	while (true)
	{
		//send();
	}

	return 0;
}

int main()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ServerSockAddr;
	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = PF_INET;
	inet_pton(AF_INET, "127.0.0.1", (void*)&ServerSockAddr.sin_addr.s_addr);
	ServerSockAddr.sin_port = htons(20200);
	connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));

	HANDLE ThreadHandles[2] = { 0, };
	ThreadHandles[0] = (HANDLE)_beginthreadex(0, 0, RecvThread, (void*)&ServerSocket, 0, 0);
	ThreadHandles[1] = (HANDLE)_beginthreadex(0, 0, SendThread, (void*)&ServerSocket, 0, 0);

	WaitForMultipleObjects(2, ThreadHandles, true, INFINITE);

	closesocket(ServerSocket);


	WSACleanup();

	return 0;
}