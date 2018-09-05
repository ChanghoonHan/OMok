#include "GameManager.h"

GameManager* GameManager::S = NULL;



void GameManager::InitGameManager(HWND hWnd, HDC hdc)
{
	m_gameState = READY;
	m_bIsMyTrun = false;
	m_bReady[0] = false;
	m_bReady[1] = false;
	memset(m_userName, '\0', 20);
	memset(m_otherUserName, '\0', 20);
	
	for (size_t i = 0; i < 19; i++)
	{
		for (size_t j = 0; j < 19; j++)
		{
			m_map[i][j] = ROCK_NULL;
		}
	}

	m_hWnd = hWnd;
	m_backGroundBitMap.Init(hdc, "gameBack.bmp");
	m_whiteRock.Init(hdc, "whiteRock.bmp");
	m_blackRock.Init(hdc, "blackRock.bmp");
	m_exitButton.Init(hdc, "exitButton1.bmp");
	m_readyButton.Init(hdc, "readyButton1.bmp");
	m_dropButton.Init(hdc, "dropButton1.bmp");
	m_readyBitMap.Init(hdc, "readyPrint.bmp");
	m_winBitMap.Init(hdc, "win.bmp");
	m_lossBitMap.Init(hdc, "loss.bmp");
}

void GameManager::OperatorInputClick(int mouseX, int mouseY, bool isRightMouse)
{
	if (mouseX> 650 && mouseX < 650 + 46 &&
		mouseY > 235 && mouseY < 235 + 46)
	{
		closesocket(m_sock);
		exit(0);
	}

	switch (m_gameState)
	{
	case WIN:
	case LOSS:
	case READY:
		if (mouseX> 530 && mouseX < 530 + 46 &&
			mouseY > 235 && mouseY < 235 + 46)
		{
			m_bReady[0] = !m_bReady[0];
			PACKET_READY readyPacket;
			readyPacket.header.packetLen = sizeof(readyPacket);
			readyPacket.header.type = PACKET_TYPE_READY;
			readyPacket.ready = m_bReady[0];
			send(m_sock, (char*)&readyPacket, sizeof(readyPacket), 0);
		}

		break;

	case GAME:
		if (!m_bIsMyTrun)
		{
			return;
		}

		for (size_t i = 0, y = 0; i < 19; i++)
		{
			if (i != 0 && i % 2 == 0)
			{
				y++;
			}

			for (size_t j = 0, x = 0; j < 19; j++)
			{
				if (j != 0 && j % 2 == 0)
				{
					x++;
				}

				if (mouseX> 18 - x + (j * 25) && mouseX < 18 - x + (j * 25) + 22 &&
					mouseY > 19 - y + (i * 25) && mouseY < 19 - y + (i * 25) + 22)
				{
					PACKET_CAN_PUT canPutPacket;
					canPutPacket.header.packetLen = sizeof(PACKET_CAN_PUT);
					canPutPacket.header.type = PACKET_TYPE_CAN_PUT;
					canPutPacket.x = j;
					canPutPacket.y = i;
					send(m_sock, (char*)&canPutPacket, sizeof(PACKET_CAN_PUT), 0);
				}
			}
		}

		break;

	default:
		break;
	}

	
}

void GameManager::Update()
{
	
}

void GameManager::DrawGame(HDC hdc)
{
	m_backGroundBitMap.Draw(hdc, 0, 0);
	m_exitButton.Draw(hdc, 650, 235);
	m_dropButton.Draw(hdc, 590, 235);
	m_readyButton.Draw(hdc, 530, 235);
	TextOut(hdc, 533, 180, m_userName, strlen(m_userName));
	TextOut(hdc, 638, 180, m_otherUserName, strlen(m_otherUserName));

	switch (m_gameState)
	{
	case READY:
		if (m_bReady[0])
		{
			TextOut(hdc, 533, 200, "READY", strlen("READY"));
		}

		if (m_bReady[1])
		{
			TextOut(hdc, 638, 200, "READY", strlen("READY"));
		}
		DrawMap(hdc);
		m_readyBitMap.Draw(hdc, 0, 0);
		break;

	case GAME:
		m_bReady[0] = false;
		m_bReady[1] = false;

		if (!m_bIsMyTrun)
		{
			TextOut(hdc, 638, 200, "TURN", strlen("TURN"));
		}
		else
		{
			TextOut(hdc, 533, 200, "TURN", strlen("TURN"));
		}
		
		DrawMap(hdc);
		break;
	case WIN:
		if (m_bReady[0])
		{
			TextOut(hdc, 533, 200, "READY", strlen("READY"));
		}

		if (m_bReady[1])
		{
			TextOut(hdc, 638, 200, "READY", strlen("READY"));
		}

		DrawMap(hdc);

		m_winBitMap.Draw(hdc, 130, 140);
		break;
	case LOSS:
		if (m_bReady[0])
		{
			TextOut(hdc, 533, 200, "READY", strlen("READY"));
		}

		if (m_bReady[1])
		{
			TextOut(hdc, 638, 200, "READY", strlen("READY"));
		}

		DrawMap(hdc);

		m_lossBitMap.Draw(hdc, 130, 130);
		break;
	default:
		break;
	}
}

void GameManager::ProcessSocketMessage(HWND hWnd, UINT uMsg, WPARAM socket, LPARAM msg)
{
	int addrlen = 0;
	int retval = 0;

	if (WSAGETSELECTERROR(msg))
	{
		err_display((char*)WSAGETSELECTERROR(msg));
		return;
	}

	switch (WSAGETSELECTEVENT(msg))
	{

	case FD_READ:
	{
		char szBuf[BUFSIZE];

		while (true)
		{
			retval = recv(socket, szBuf, BUFSIZE, 0);
			if (retval == SOCKET_ERROR)
			{
				if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					err_display("WSAAsyncSelect()");

				}
				continue;
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

			if (m_recvlen >= sizeof(PACKET_HEADER))//더온경우
			{
				continue;
			}

			if (m_recvlen == 0)
			{
				break;
			}
		}
	}
	break;

	case FD_CLOSE:
		closesocket(socket);
		break;
	}
}

bool GameManager::ProcessPacket(HWND hWnd, SOCKET sock, char* buf, int retval)
{
	memcpy(&m_userBuf[m_recvlen], buf, retval);
	m_recvlen += retval;

	if (m_recvlen < sizeof(PACKET_HEADER))
	{
		return false;
	}

	PACKET_HEADER header;
	memcpy(&header, buf, sizeof(header));

	switch (header.type)
	{
	case PACKET_TYPE_READY:
	{
		PACKET_READY readyPacket;
		memcpy(&readyPacket, buf, header.packetLen);
		m_bReady[1] = readyPacket.ready;
		InvalidateRect(hWnd, NULL, TRUE);
	}
		break;

	case PACKET_TYPE_START:
	{
		for (size_t i = 0; i < 19; i++)
		{
			for (size_t j = 0; j < 19; j++)
			{
				m_map[i][j] = ROCK_NULL;
			}
		}
		PACKET_START startPacket;
		memcpy(&startPacket, buf, header.packetLen);
		m_gameState = GAME;
		m_bIsMyTrun = startPacket.turn;
		InvalidateRect(hWnd, NULL, TRUE);
	}
	break;

	case PACKET_TYPE_LOGIN_RESULT:
	{
		PACKET_LOGIN_RESULT loginResultPacket;
		memcpy(&loginResultPacket, buf, header.packetLen);

		strcpy(m_userName, loginResultPacket.userID);

		InvalidateRect(hWnd, NULL, TRUE);
	}
		break;

	case PACKET_TYPE_MAP_INFO:
	{
		PACKET_MAP_INFO mapInfoPacket;
		memcpy(&mapInfoPacket, buf, header.packetLen);

		memcpy(m_map, mapInfoPacket.map, sizeof(ROCK)*19*19);

		m_bIsMyTrun = mapInfoPacket.turn;
		switch (mapInfoPacket.gameResult)
		{
		case GAMERESULT_NULL:
			break;
		case GAMERESULT_WIN:
			m_gameState = WIN;
			break;
		case GAMERESULT_LOSS:
			m_gameState = LOSS;
			break;
		default:
			break;
		}
		InvalidateRect(hWnd, NULL, TRUE);
	}
	break;

	case PACKET_TYPE_PLAYERS_INFO:
	{
		PACKET_PLAYERS_INFO playersInfoPacket;
		memcpy(&playersInfoPacket, buf, header.packetLen);

		if (m_gameState == GAME)
		{
			m_gameState = WIN;
		}

		for (size_t i = 0; i < 2; i++)
		{
			if (!strcmp(playersInfoPacket.userID[i], m_userName))
			{
				continue;
			}
			else
			{
				strcpy(m_otherUserName, playersInfoPacket.userID[i]);
			}
		}

		InvalidateRect(hWnd, NULL, TRUE);
	}
		break;
	}

	memcpy(m_userBuf, & m_userBuf[m_recvlen - header.packetLen],m_recvlen - header.packetLen);
	m_recvlen -= header.packetLen;
}

void GameManager::SetSock(SOCKET sock)
{
	m_sock = sock;
}

void GameManager::err_display(char *msg)//에러출력
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	//printf("[%s] %s", msg, lpMsgBuf);
	LocalFree(lpMsgBuf);
}

GameManager* GameManager::GetInstance()
{
	if (S == NULL)
	{
		S = new GameManager();
	}

	return S;
}

void GameManager::Release()
{
	m_backGroundBitMap.Release();


	delete S;
}

GameManager::GameManager()
{
}

GameManager::~GameManager()
{
}

void GameManager::DrawMap(HDC hdc)
{
	for (size_t i = 0, y = 0; i < 19; i++)
	{
		if (i != 0 && i % 2 == 0)
		{
			y++;
		}

		for (size_t j = 0, x = 0; j < 19; j++)
		{
			if (j != 0 && j % 2 == 0)
			{
				x++;
			}

			switch (m_map[i][j])
			{
			case ROCK_NULL:
				break;
			case ROCK_BLACK:
				m_blackRock.Draw(hdc, (18 - x + (j * 25)), (19 - y + (i * 25)));
				break;
			case ROCK_WHITE:
				m_whiteRock.Draw(hdc, (18 - x + (j * 25)), (19 - y + (i * 25)));
				break;
			default:
				break;
			}
		}
	}
}