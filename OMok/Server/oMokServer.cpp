#include <WinSock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <map>
#include <random>
#include "..\..\packetHeader.h"

using namespace std;

struct PLAYERBUF
{
	char userId[20];
	int len;
	char userBuf[BUFSIZE];
	bool ready;
};
SOCKET g_client_sock;
SOCKADDR_IN g_clientaddr;
PACKET_PLAYERS_INFO g_playersInfo;

std::map<SOCKET, PLAYERBUF*> g_mapPlayer;
int g_playerCount = 0;
int g_playerIdCount = 0;
bool g_bRockColor = true;
ROCK g_map[19][19];

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void ProcessSocketMessage(HWND, UINT, WPARAM, LPARAM);
bool ProcessPacket(HWND hWnd, SOCKET sock, char* buf, int retval);
void InitMap();
bool TestGameOver(int x, int y);

void err_quit(char *msg)//에러 출력 후 종료 //심각한 오류 발생시
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(-1);
}

void err_display(char *msg)//에러출력
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

void err_display(int errcode)//에러출력
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[오류] %s", (LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
}


int main(int argc, char* argv[])
{
	srand(GetTickCount());

	WNDCLASS WndClass;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;//통산 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//미리만들어놓은 브러쉬 가져옴
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = NULL;
	WndClass.lpfnWndProc = WndProc;//프로시저 받아옴 메세지 재정의를 모아놓은 함수
	WndClass.lpszClassName = "MyWindowClass";
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;//윈도우 스타일

	if (!RegisterClass(&WndClass))
	{
		return -1;
	}//os에 윈도우 등록

	HWND hWnd = CreateWindow("MyWindowClass", "TCP server", WS_OVERLAPPEDWINDOW,
		0, 0, 600, 300,
		NULL, (HMENU)NULL, NULL, NULL);

	if (hWnd == NULL)
	{
		return -1;
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);//한번 그려옴
	UpdateWindow(hWnd);

	//////////////////////////////
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);//접속 대기용 소켓
	if (listen_sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	retval = WSAAsyncSelect(listen_sock, hWnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE);
	if (retval == SOCKET_ERROR)
	{
		err_quit("WSAAsyncSelect()");
	}

	SOCKADDR_IN serveraddr;//자기 정보 셋팅(바인드)
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("bind()");
	}

	retval = listen(listen_sock, SOMAXCONN);//리슨상태로 바꿔줌
	if (retval == SOCKET_ERROR)
	{
		err_quit("listen()");
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_SOCKET:
		ProcessSocketMessage(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void ProcessSocketMessage(HWND hWnd, UINT uMsg, WPARAM socket, LPARAM msg)
{
	int addrlen = 0;
	int retval = 0;

	if (WSAGETSELECTERROR(msg))
	{
		//err_display((char*)WSAGETSELECTERROR(msg));
		g_playerCount--;
		delete g_mapPlayer[socket];
		g_mapPlayer.erase(socket);
		closesocket(socket);

		g_playersInfo.header.packetLen = sizeof(PACKET_PLAYERS_INFO);
		g_playersInfo.header.type = PACKET_TYPE_PLAYERS_INFO;

		int i = 0;
		strcpy(g_playersInfo.userID[0], "");
		strcpy(g_playersInfo.userID[1], "");
		for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
		{
			strcpy(g_playersInfo.userID[i], iter->second->userId);
			i++;
		}

		for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
		{
			while (true)
			{
				send(iter->first, (char*)&g_playersInfo, g_playersInfo.header.packetLen, 0);
				if (retval == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						err_display("send()");
						closesocket(iter->first);
						delete(iter->second);
					}
					Sleep(10);
					continue;
				}
				break;
			}
		}
		return;
	}

	switch (WSAGETSELECTEVENT(msg))
	{
	case FD_ACCEPT:
	{
		addrlen = sizeof(g_clientaddr);
		g_client_sock = accept(socket, (SOCKADDR*)&g_clientaddr, &addrlen);
		if (g_client_sock == INVALID_SOCKET)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				err_display("accept()");
				closesocket(g_client_sock);
			}
			return;
		}

		printf("\n[TCP 서버] 클라이언트 접속 : IP 주소 = %s, 포트번호 = %d\n",
			inet_ntoa(g_clientaddr.sin_addr), ntohs(g_clientaddr.sin_port));

		retval = WSAAsyncSelect(g_client_sock, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);
		if (retval == SOCKET_ERROR)
		{
			err_display("WSAAsyncSelect()");
			closesocket(g_client_sock);
			return;
		}
		g_playerCount++;
		if (g_playerCount > 2)
		{
			closesocket(g_client_sock);
			return;
		}

		

		PLAYERBUF* playerBuf = new PLAYERBUF;
		playerBuf->len = 0;
		playerBuf->ready = false;

		PACKET_LOGIN_RESULT lResult;
		lResult.header.packetLen = sizeof(lResult);
		lResult.header.type = PACKET_TYPE_LOGIN_RESULT;
		
		sprintf(lResult.userID, "%s%d", "USER ", g_playerIdCount++);
		strcpy(playerBuf->userId, lResult.userID);

		retval = send(g_client_sock, (char*)&lResult, sizeof(lResult), 0);
		if (retval == SOCKET_ERROR)
		{
			if (WSAGetLastError() != WSAEWOULDBLOCK)
			{
				err_display("send()");
				closesocket(g_client_sock);
				delete(playerBuf);
			}
			return;
		}

		g_mapPlayer.insert(make_pair(g_client_sock, playerBuf));
		Sleep(100);

		//유저들에게 다 보내주기
		g_playersInfo.header.packetLen = sizeof(PACKET_PLAYERS_INFO);
		g_playersInfo.header.type = PACKET_TYPE_PLAYERS_INFO;

		int i = 0;
		for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
		{
			strcpy(g_playersInfo.userID[i], iter->second->userId);
			i++;
		}

		for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
		{
			while (true)
			{
				send(iter->first, (char*)&g_playersInfo, g_playersInfo.header.packetLen, 0);
				if (retval == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						err_display("send()");
						closesocket(iter->first);
						delete(iter->second);
					}
					Sleep(10);
					continue;
				}
				break;
			}
		}
	}
	break;

	case FD_READ:
	{
		char szBuf[BUFSIZE];

		while (true)
		{
			retval = recv(socket, szBuf, BUFSIZ, 0);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					//cout << "err on recv!!" << endl;
					err_display("err on recv!!");
				}
				else
				{
					Sleep(10);
					continue;
				}
			}
			break;
		}

		while (true)
		{

			if (ProcessPacket(hWnd, socket, szBuf, retval) == false)
			{
				SendMessage(hWnd, WM_SOCKET, socket, msg);
				break;
			}

			if (g_mapPlayer[socket]->len >= sizeof(PACKET_HEADER))//더온경우
			{
				continue;
			}

			if (g_mapPlayer[socket]->len == 0)
			{
				break;
			}
		}

	}
	break;

	case FD_CLOSE:
		g_playerCount--;
		delete g_mapPlayer[socket];
		g_mapPlayer.erase(socket);
		closesocket(socket);

		g_playersInfo.header.packetLen = sizeof(PACKET_PLAYERS_INFO);
		g_playersInfo.header.type = PACKET_TYPE_PLAYERS_INFO;

		int i = 0;
		strcpy(g_playersInfo.userID[0], "");
		strcpy(g_playersInfo.userID[1], "");
		for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
		{
			strcpy(g_playersInfo.userID[i], iter->second->userId);
			i++;
		}

		for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
		{
			while (true)
			{
				send(iter->first, (char*)&g_playersInfo, g_playersInfo.header.packetLen, 0);
				if (retval == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						err_display("send()");
						closesocket(iter->first);
						delete(iter->second);
					}
					Sleep(10);
					continue;
				}
				break;
			}
		}
		break;
	}
}

bool ProcessPacket(HWND hWnd, SOCKET sock, char* buf, int retval)
{
	PLAYERBUF* playerBuf = g_mapPlayer[sock];
	memcpy(&playerBuf->userBuf[playerBuf->len], buf, retval);
	playerBuf->len += retval;

	if (playerBuf->len < sizeof(PACKET_HEADER))
	{
		return false;
	}

	PACKET_HEADER header;
	memcpy(&header, buf, sizeof(header));

	if (playerBuf->len < header.packetLen)
	{
		return false;
	}

	switch (header.type)
	{
	case PACKET_TYPE_READY:
	{
		PACKET_READY readyPacket;
		memcpy(&readyPacket, buf, sizeof(readyPacket));

		auto g_mapiter = g_mapPlayer.find(sock);
		g_mapiter->second->ready = readyPacket.ready;

		for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
		{
			if (iter->first == sock)
			{
				continue;
			}

			send(iter->first, (char*)&readyPacket, sizeof(readyPacket), 0);
			while (1)
			{
				if (retval == SOCKET_ERROR)
				{
					if (WSAGetLastError() != WSAEWOULDBLOCK)
					{
						err_display("send()");
						delete g_mapPlayer[iter->first]->userBuf;
						g_mapPlayer.erase(iter->first);
					}

					continue;
				}
				break;
			}
		}

		bool allReady = true;
		for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
		{
			if (!iter->second->ready)
			{
				allReady = false;
				break;
			}
		}

		if (allReady)
		{
			Sleep(1000);
			PACKET_START startPacket;
			startPacket.header.packetLen = sizeof(startPacket);
			startPacket.header.type = PACKET_TYPE_START;
			startPacket.turn = false;

			for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
			{
				if (iter->first == sock)
				{
					startPacket.turn = false;
				}
				else
				{
					startPacket.turn = true;
				}

				send(iter->first, (char*)&startPacket, sizeof(startPacket), 0);
				while (1)
				{
					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSAEWOULDBLOCK)
						{
							err_display("send()");
							delete g_mapPlayer[iter->first]->userBuf;
							g_mapPlayer.erase(iter->first);
						}

						continue;
					}
					break;
				}
			}

			InitMap();
			g_bRockColor = true;

			for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
			{
				iter->second->ready = false;
			}
		}
	}
		break;

	case PACKET_TYPE_CAN_PUT:
	{
		PACKET_CAN_PUT userCanPutPacket;
		memcpy(&userCanPutPacket, buf, sizeof(PACKET_CAN_PUT));

		bool canPut = false;
		bool isGameOver = false;
		if (g_map[userCanPutPacket.y][userCanPutPacket.x] == ROCK_NULL)
		{
			if(g_bRockColor)
			{ 
				g_map[userCanPutPacket.y][userCanPutPacket.x] = ROCK_BLACK;
			}
			else
			{
				g_map[userCanPutPacket.y][userCanPutPacket.x] = ROCK_WHITE;
			}
			g_bRockColor = !g_bRockColor;
			canPut = true;
		}	

		isGameOver = TestGameOver(userCanPutPacket.x, userCanPutPacket.y);

		if (canPut)
		{
			PACKET_MAP_INFO mapInfoPacket;
			mapInfoPacket.header.packetLen = sizeof(PACKET_MAP_INFO);
			mapInfoPacket.header.type = PACKET_TYPE_MAP_INFO;
			mapInfoPacket.gameResult = GAMERESULT_NULL;
			mapInfoPacket.turn = false;
			memcpy(mapInfoPacket.map, g_map, sizeof(ROCK) * 19 * 19);

			for (auto iter = g_mapPlayer.begin(); iter != g_mapPlayer.end(); iter++)
			{
				if (iter->first == sock)
				{
					if (isGameOver)
					{
						mapInfoPacket.gameResult = GAMERESULT_WIN;
					}
					mapInfoPacket.turn = false;
				}
				else
				{
					if (isGameOver)
					{
						mapInfoPacket.gameResult = GAMERESULT_LOSS;
					}
					mapInfoPacket.turn = true;
				}

				send(iter->first, (char*)&mapInfoPacket, sizeof(PACKET_MAP_INFO), 0);
				while (1)
				{
					if (retval == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSAEWOULDBLOCK)
						{
							err_display("send()");
							delete g_mapPlayer[sock]->userBuf;
							g_mapPlayer.erase(sock);
						}

						continue;
					}
					break;
				}
			}
		}
	}
		break;
	}

	memcpy(playerBuf->userBuf, &playerBuf->userBuf[playerBuf->len - header.packetLen], playerBuf->len - header.packetLen);
	playerBuf->len -= header.packetLen;

	return true;
}

void InitMap()
{
	for (size_t i = 0; i < 19; i++)
	{
		for (size_t j = 0; j < 19; j++)
		{
			g_map[i][j] = ROCK_NULL;
		}
	}
}

bool TestGameOver(int x, int y)
{
	ROCK rock = g_map[y][x];

	bool isGameOver = false;
	int search = 0;
	int searchX = x;
	int searchY = y;
	
	ROCK curRock;

	int tempX;
	int tempY;

	int direct[8][2] = { { -1, 0 },{ -1, 1 },{ 0, 1 },{ 1, 1 },{ 1, 0 },{ 1, -1 },{ 0 ,-1 },{ -1, -1 } };

	for (size_t k = 0; k < 4; k++)
	{
		searchX = x;
		searchY = y;

		for (size_t i = 0; i < 4; i++)
		{
			isGameOver = true;
			tempX = searchX += direct[k][1];
			tempY = searchY += direct[k][0];

			if (tempX < 0 || tempX > 18 || tempY < 0 || tempY > 18)
			{
				isGameOver = false;
				break;
			}

			curRock = g_map[tempY][tempX];
			if (curRock != rock)
			{
				isGameOver = false;
				break;
			}

			if (i == 3)
			{
				return isGameOver;
			}
		}

		for (size_t i = 0; i < 5; i++)
		{
			isGameOver = true;

			tempX = searchX += direct[k + 4][1];
			tempY = searchY += direct[k + 4][0];

			if (tempX < 0 || tempX > 18 || tempY < 0 || tempY > 18)
			{
				isGameOver = false;
				break;
			}

			curRock = g_map[tempY][tempX];
			if (curRock != rock)
			{
				isGameOver = false;
				break;
			}

			if (i == 4)
			{
				return isGameOver;
			}
		}
	}
	
	return isGameOver;
}