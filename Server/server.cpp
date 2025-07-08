#include <iostream>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

int main()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ListenSockAddr;
	memset(&ListenSockAddr, 0, sizeof(ListenSockAddr));
	ListenSockAddr.sin_family = PF_INET;
	ListenSockAddr.sin_addr.s_addr = INADDR_ANY;
	ListenSockAddr.sin_port = htons(20200);
	bind(ListenSocket, (SOCKADDR*)&ListenSockAddr, sizeof(ListenSockAddr));
	listen(ListenSocket, 5);

	TIMEVAL Timeout = TIMEVAL{ 0, 100 };
	fd_set ReadSockets;
	fd_set ReadSocketCopy;
	FD_ZERO(&ReadSockets);
	FD_SET(ListenSocket, &ReadSockets);

	while (true)
	{
		ReadSocketCopy = ReadSockets;

		int ChangeSocketCount = select(0, &ReadSocketCopy, 0, 0, &Timeout);
		if (ChangeSocketCount > 0)
		{
			for (int i = 0; i < (int)ReadSockets.fd_count; ++i)
			{
				if (FD_ISSET(ReadSockets.fd_array[i], &ReadSocketCopy))
				{
					if (ReadSockets.fd_array[i] == ListenSocket)
					{
						SOCKADDR_IN ClientSockAddr;
						int ClientSockAddrLength = sizeof(ClientSockAddr);
						SOCKET ClientSocket = accept(ListenSocket, (SOCKADDR*)&ClientSockAddr, &ClientSockAddrLength);
						FD_SET(ClientSocket, &ReadSockets);
						std::cout << "Connected Client." << std::endl;
					}
					else
					{
						//recv
					}
				}
			}
		}
	}


	closesocket(ListenSocket);


	WSACleanup();

	return 0;
}