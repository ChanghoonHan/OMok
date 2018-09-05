#pragma once
#include "BitMap.h"
#include "..\..\packetHeader.h"

enum GAMESTATE
{
	READY,
	GAME,
	WIN,
	LOSS,
};

class GameManager
{
	HWND m_hWnd;
	GAMESTATE m_gameState;
	BitMap m_backGroundBitMap;
	BitMap m_readyBitMap;
	BitMap m_whiteRock;
	BitMap m_blackRock;
	BitMap m_exitButton;
	BitMap m_readyButton;
	BitMap m_dropButton;
	BitMap m_winBitMap;
	BitMap m_lossBitMap;

	SOCKET m_sock;

	bool m_bReady[2];
	bool m_bIsMyTrun;

	char m_userName[20];
	char m_otherUserName[20];
	char m_userBuf[BUFSIZE];
	int m_recvlen = 0;
	
	ROCK m_map[19][19];

	static GameManager* S;
	GameManager();
	void err_display(char *msg);
	bool ProcessPacket(HWND hWnd, SOCKET sock, char* buf, int retval);
	void DrawMap(HDC hdc);
public:
	void Update();
	void OperatorInputClick(int x, int y, bool isRightMouse);
	void InitGameManager(HWND hWnd, HDC hdc);
	void DrawGame(HDC hdc);
	void ProcessSocketMessage(HWND, UINT, WPARAM, LPARAM);
	void SetSock(SOCKET sock);

	static GameManager* GetInstance();
	void Release();

	~GameManager();
};

