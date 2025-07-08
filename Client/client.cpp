#define NOMINMAX

#include <iostream>
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include <thread>
#include <mutex>
#include "Common.h"
#include "UserEvents_generated.h"

#pragma comment(lib, "ws2_32")

CRITICAL_SECTION SessionCS;

std::mutex SessionMutex;


int ProcessPacket(SOCKET ServerSocket, const char* Buffer);

uint32_t PlayerID = 0;

//unsigned RecvThread(void* Arg)
void RecvThread(SOCKET ServerSocket)
{
	//SOCKET ServerSocket = *(SOCKET*)Arg;

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

	//return 0;
}

//unsigned SendThread(void* Arg)
void SendThread(SOCKET ServerSocket)
{
	//SOCKET ServerSocket = *(SOCKET*)Arg;

	while (true)
	{
		if (_kbhit())
		{
			int KeyCode = _getch();
			flatbuffers::FlatBufferBuilder SendBuilder;
			auto PlayerMoveData = UserEvents::CreateC2S_PlayerMoveData(SendBuilder, PlayerID,
				SessionList[PlayerID].X,
				SessionList[PlayerID].Y,
				KeyCode);
			auto EventData = UserEvents::CreateEventData(SendBuilder, GetTimeStamp(), UserEvents::EventType_C2S_PlayerMoveData, PlayerMoveData.Union());
			SendBuilder.Finish(EventData);
			SendPacket(ServerSocket, SendBuilder);
		}
		else
		{
			//EnterCriticalSection(&SessionCS);
			////draw
			//for (const auto& SelectecSession : SessionList)
			//{
			//	GotoXY(SelectecSession.second.X, SelectecSession.second.Y);
			//	std::cout << SelectecSession.second.PlayerSocket;
			//}
			//LeaveCriticalSection(&SessionCS);

			SessionMutex.lock();
			//draw
			for (const auto& SelectecSession : SessionList)
			{
				GotoXY(SelectecSession.second.X, SelectecSession.second.Y);
				std::cout << SelectecSession.second.PlayerSocket;
			}
			SessionMutex.unlock();
		}
	}

	//return 0;
}

int main()
{
	InitializeCriticalSection(&SessionCS);

	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	SOCKET ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ServerSockAddr;
	memset(&ServerSockAddr, 0, sizeof(ServerSockAddr));
	ServerSockAddr.sin_family = PF_INET;
	inet_pton(AF_INET, "127.0.0.1", (void*)&ServerSockAddr.sin_addr.s_addr);
	ServerSockAddr.sin_port = htons(20200);
	connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));

	//Login
	flatbuffers::FlatBufferBuilder SendBuilder;

	auto LoginEvent = UserEvents::CreateC2S_Login(SendBuilder, SendBuilder.CreateString("userid"), SendBuilder.CreateString("password"));
	auto EventData = UserEvents::CreateEventData(SendBuilder, GetTimeStamp(), UserEvents::EventType_C2S_Login, LoginEvent.Union());
	SendBuilder.Finish(EventData);

	SendPacket(ServerSocket, SendBuilder);

	//HANDLE ThreadHandles[2] = { 0, };
	//ThreadHandles[0] = (HANDLE)_beginthreadex(0, 0, RecvThread, (void*)&ServerSocket, 0, 0);
	//ThreadHandles[1] = (HANDLE)_beginthreadex(0, 0, SendThread, (void*)&ServerSocket, 0, 0);

	//WaitForMultipleObjects(2, ThreadHandles, true, INFINITE);

	std::thread STLRecvThread(RecvThread, ServerSocket);
	std::thread STLSendThread(SendThread, ServerSocket);

	STLRecvThread.join();
	STLSendThread.join();

	closesocket(ServerSocket);


	WSACleanup();

	DeleteCriticalSection(&SessionCS);

	return 0;
}

int ProcessPacket(SOCKET ServerSocket, const char* RecvBuffer)
{
	flatbuffers::FlatBufferBuilder SendBuilder;
	//root_type
	auto RecvEventData = UserEvents::GetEventData(RecvBuffer);
	//std::cout << RecvEventData->timestamp() << std::endl; //타임스탬프

	switch (RecvEventData->data_type())
	{
		case UserEvents::EventType_S2C_Login:
		{
			auto LoginData = RecvEventData->data_as_S2C_Login();
			if (LoginData)
			{
				std::cout << "로그인 성공: " << LoginData->success() << std::endl;
				std::cout << "유저 ID: " << LoginData->player_id() << std::endl;
				PlayerID = LoginData->player_id();
			}
			else
			{
				std::cout << "로그인 데이터가 유효하지 않습니다." << std::endl;
			}
		}
		break;
		case UserEvents::EventType_S2C_PlayerMoveData:
		{
			auto PlyerMoveData = RecvEventData->data_as_S2C_PlayerMoveData();
			if (PlyerMoveData)
			{
				EnterCriticalSection(&SessionCS);
				SessionList[PlyerMoveData->player_id()].X = PlyerMoveData->position_x();
				SessionList[PlyerMoveData->player_id()].Y = PlyerMoveData->position_y();
				LeaveCriticalSection(&SessionCS);
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
		case UserEvents::EventType_S2C_SpawnPlayer:
		{
			auto LoginData = RecvEventData->data_as_S2C_SpawnPlayer();

			EnterCriticalSection(&SessionCS);
			SessionList[LoginData->player_id()] = Session(LoginData->player_id(),
				LoginData->position_x(), LoginData->position_y(),
				LoginData->message()->c_str(), *LoginData->color());
			LeaveCriticalSection(&SessionCS);
		}
		break;
		case UserEvents::EventType_S2C_DestroyPlayer:
		{
			auto DestroyPlayerData = RecvEventData->data_as_S2C_DestroyPlayer();
			EnterCriticalSection(&SessionCS);
			SessionList.erase(DestroyPlayerData->player_id());
			LeaveCriticalSection(&SessionCS);
		}
		break;
	}

	return 0;
}
