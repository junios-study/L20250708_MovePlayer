#define NOMINMAX

#include <iostream>
#include <WinSock2.h>

#include "Common.h"
#include "UserEvents_generated.h"



#pragma comment(lib, "ws2_32")
int ProcessPacket(SOCKET ClientSocket, const char* Buffer);

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
						char Buffer[4096] = { 0, };
						int RecvBytes = RecvPacket(ReadSockets.fd_array[i], Buffer);
						if (RecvBytes <= 0)
						{
							closesocket(ReadSockets.fd_array[i]);
							FD_CLR(ReadSockets.fd_array[i], &ReadSockets);
						}
						else
						{
							//ProcessPacket();
						}
					}
				}
			}
		}
	}


	closesocket(ListenSocket);


	WSACleanup();

	return 0;
}



int ProcessPacket(SOCKET ClientSocket, const char* RecvBuffer)
{
	//root_type
	auto RecvEventData = UserEvents::GetEventData(RecvBuffer);
	std::cout << RecvEventData->timestamp() << std::endl; //Å¸ÀÓ½ºÅÆÇÁ

	flatbuffers::FlatBufferBuilder SendBuilder;

	switch (RecvEventData->data_type())
	{
	case UserEvents::EventType_C2S_Login:
	{
		auto LoginData = RecvEventData->data_as_C2S_Login();
		if (LoginData->userid() && LoginData->password())
		{
			std::cout << "Login Request success: " << LoginData->userid()->c_str() << ", " << LoginData->password()->c_str() << std::endl;
			auto LoginEvent = UserEvents::CreateS2C_Login(SendBuilder, (uint32_t)ClientSocket, true, SendBuilder.CreateString("Login Success"), 10, 10, nullptr);
			auto EventData = UserEvents::CreateEventData(SendBuilder, GetTimeStamp(), UserEvents::EventType_S2C_Login, LoginEvent.Union());
			SendBuilder.Finish(EventData);
		}
		else
		{
			auto LoginEvent = UserEvents::CreateS2C_Login(SendBuilder, (uint32_t)ClientSocket, false, SendBuilder.CreateString("empty id, password"), 10, 10, nullptr);
			auto EventData = UserEvents::CreateEventData(SendBuilder, GetTimeStamp(), UserEvents::EventType_S2C_Login, LoginEvent.Union());
			SendBuilder.Finish(EventData);
		}
		SendPacket(ClientSocket, SendBuilder);
	}
	break;
	case UserEvents::EventType_C2S_PlayerMoveData:
	{
		std::cout << "EventType_C2S_PlayerMoveData" << std::endl;
		auto PlayerMoveData = RecvEventData->data_as_C2S_PlayerMoveData();
		int PlayerX = PlayerMoveData->position_x();
		int PlayerY = PlayerMoveData->position_y();
		if (PlayerMoveData->key_code() != 0)
		{
			switch (toupper(PlayerMoveData->key_code()))
			{
				case 'W':
				{
					PlayerY--;
					break;
				}
				case 'S':
				{
					PlayerY++;
					break;
				}
				case 'A':
				{
					PlayerX--;
					break;
				}
				case 'D':
				{
					PlayerX++;
					break;
				}
				case 27:
				{
					auto LogoutEvent = UserEvents::CreateS2C_Logout(SendBuilder, (uint32_t)ClientSocket);
					auto EventData = UserEvents::CreateEventData(SendBuilder, GetTimeStamp(), UserEvents::EventType_S2C_Logout, LogoutEvent.Union());
					SendBuilder.Finish(EventData);
					SendPacket(ClientSocket, SendBuilder);
					break;
				}
			}

			PlayerX = std::clamp(PlayerX, 0, 100);
			PlayerY = std::clamp(PlayerY, 0, 100);
		}

		GotoXY(PlayerX, PlayerY);
		std::cout << "P" << std::endl;

		auto SendPlayerMoveData = UserEvents::CreateS2C_PlayerMoveData(SendBuilder, (uint32_t)ClientSocket, PlayerX, PlayerY);
		auto EventData = UserEvents::CreateEventData(SendBuilder, GetTimeStamp(), UserEvents::EventType_S2C_PlayerMoveData, SendPlayerMoveData.Union());
		SendBuilder.Finish(EventData);
	}
	break;
	case UserEvents::EventType_C2S_Logout:
	{
		auto LoginData = RecvEventData->data_as_S2C_Logout();
		return -1;
	}
	break;
	}

	return 0;
}