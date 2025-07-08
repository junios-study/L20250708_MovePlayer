#define NOMINMAX

#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include "Common.h"
#include "UserEvents_generated.h"

#pragma comment(lib, "ws2_32")

int ProcessPacket(SOCKET ServerSocket, const char* Buffer);

unsigned RecvThread(void* Arg)
{
	SOCKET ServerSocket = *(SOCKET*)Arg;

	while (true)
	{
		char Buffer[4096];
		int RecvBytes = RecvPacket(ServerSocket, Buffer);
		if (RecvBytes <= 0)
		{
			shutdown(ServerSocket, SD_BOTH);
			closesocket(ServerSocket);
			break;
		}
		ProcessPacket(ServerSocket, Buffer);
	}

	return 0;
}

unsigned SendThread(void* Arg)
{
	SOCKET ServerSocket = *(SOCKET*)Arg;

	while (true)
	{
		if (!_kbhit())
		{
			int KeyCode = _getch();
			//SendPacket()

			//auto PlayerMoveData = UserEvents::CreateC2S_PlayerMoveData(SendBuilder, PlyerMoveData->position_x(), PlyerMoveData->position_y(), keyCode);
		}
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

int ProcessPacket(SOCKET ServerSocket, const char* RecvBuffer)
{
	flatbuffers::FlatBufferBuilder SendBuilder;
	//root_type
	auto RecvEventData = UserEvents::GetEventData(RecvBuffer);
	std::cout << RecvEventData->timestamp() << std::endl; //타임스탬프

	switch (RecvEventData->data_type())
	{
		case UserEvents::EventType_S2C_Login:
		{
			auto LoginData = RecvEventData->data_as_S2C_Login();
			if (LoginData)
			{
				std::cout << "로그인 성공: " << LoginData->success() << std::endl;
				std::cout << "유저 ID: " << LoginData->player_id() << std::endl;
			}
			else
			{
				std::cout << "로그인 데이터가 유효하지 않습니다." << std::endl;
			}
		}
		break;
		case UserEvents::EventType_S2C_PlayerMoveData:
		{
			std::cout << "EventType_S2C_PlayerMoveData" << std::endl;
			auto PlyerMoveData = RecvEventData->data_as_S2C_PlayerMoveData();
			if (PlyerMoveData)
			{
				GotoXY(PlyerMoveData->position_x(), PlyerMoveData->position_y());
				std::cout << "P" << std::endl;
			}
			else
			{
				std::cout << "플레이어 이동 데이터가 유효하지 않습니다." << std::endl;
			}
		}
		break;
		case UserEvents::EventType_S2C_Logout:
		{
			return -1;
		}
		break;
	}

	return 0;
}
