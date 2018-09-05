#pragma once

#pragma pack(1)

#define BUFSIZE 2048
#define WM_SOCKET (WM_USER+1)

enum ROCK
{
	ROCK_NULL,
	ROCK_BLACK,
	ROCK_WHITE,
};

enum GAMERESULT
{
	GAMERESULT_NULL,
	GAMERESULT_WIN,
	GAMERESULT_LOSS,
};

enum PACKET_TYPE
{
	PACKET_TYPE_LOGIN_RESULT,
	PACKET_TYPE_MAP_INFO,
	PACKET_TYPE_START,
	PACKET_TYPE_CAN_PUT,
	PACKET_TYPE_PLAYERS_INFO,
	PACKET_TYPE_READY,
};

struct PACKET_HEADER
{
	PACKET_TYPE type;
	WORD packetLen;
};

struct PACKET_START
{
	PACKET_HEADER header;
	bool turn;
};

struct PACKET_LOGIN_RESULT
{
	PACKET_HEADER header;
	char userID[20];
};

struct PACKET_PLAYERS_INFO
{
	PACKET_HEADER header;
	char userID[2][20];
};

struct PACKET_CAN_PUT
{
	PACKET_HEADER header;
	int x;
	int y;
};

struct PACKET_MAP_INFO
{
	PACKET_HEADER header;
	ROCK map[19][19];
	GAMERESULT gameResult;
	bool turn = 0;
};

struct PACKET_READY
{
	PACKET_HEADER header;
	bool ready;
};

#pragma pack()