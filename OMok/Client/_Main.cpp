#include <Windows.h>
#include <crtdbg.h>//메모리 릭검사
#include "GameManager.h"
#include "..\..\packetHeader.h"

void err_quit(char *msg);
void err_display(char *msg);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

SOCKET client_sock;
HINSTANCE g_hInst;

char lpszClass[256] = "OMOK";

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstace,
	LPSTR lpszCmdParam, int nCmdShow)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtDumpMemoryLeaks();
	//_CrtSetBreakAlloc(142);

	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;//통산 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//미리만들어놓은 브러쉬 가져옴
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;//프로시저 받아옴 메세지 재정의를 모아놓은 함수
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;//윈도우 스타일

	RegisterClass(&WndClass);//os에 윈도우 등록

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 716 + 16, 538 + 39,
		NULL, (HMENU)NULL, hInstance, NULL);
	//CW_ ->x y wid hei
	//부모윈도우, idset, instand, NULL

	ShowWindow(hWnd, nCmdShow);//한번 그려옴


	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}

	client_sock = socket(AF_INET, SOCK_STREAM, 0);//접속용 소켓
	if (client_sock == INVALID_SOCKET)
	{
		err_quit("socket()");
	}

	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9000);
	serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	retval = connect(client_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR)
	{
		err_quit("connect()");
	}

	retval = WSAAsyncSelect(client_sock, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);
	if (retval == SOCKET_ERROR)
	{
		err_quit("WSAAsyncSelect()");
	}

	GameManager::GetInstance()->SetSock(client_sock);

	while (GetMessage(&Message, NULL, 0, 0))
	{
		TranslateMessage(&Message);//메세지 번역 큐에서 가져와서 번역 파생처리
		DispatchMessage(&Message);//윈도우 프로시저가 콜 됨
	}

	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
//핸들, 어떤메세지, 부가정보1, 부가정보2
{
	PAINTSTRUCT ps;
	HDC hdc;
	int mousePosX, mousePosY;

	switch (iMessage)
	{
	case WM_SOCKET:
		GameManager::GetInstance()->ProcessSocketMessage(hWnd, iMessage, wParam, lParam);
		return 0;

	case WM_CREATE:
		hdc = BeginPaint(hWnd, &ps);
		GameManager::GetInstance()->InitGameManager(hWnd, hdc);
		EndPaint(hWnd, &ps);
		return 0;

	case WM_TIMER:
		GameManager::GetInstance()->Update();
		InvalidateRect(hWnd, NULL, 0);
		return 0;

	case WM_LBUTTONDOWN:
		mousePosX = LOWORD(lParam);
		mousePosY = HIWORD(lParam);
		GameManager::GetInstance()->OperatorInputClick(mousePosX, mousePosY, false);
		InvalidateRect(hWnd, NULL, 0);
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		GameManager::GetInstance()->DrawGame(hdc);
		EndPaint(hWnd, &ps);
		return 0;

	case WM_DESTROY:
		GameManager::GetInstance()->Release();
		PostQuitMessage(0);//큐에 나간다는메세지 적재
		return 0;
		break;
	}

	return(DefWindowProc(hWnd, iMessage, wParam, lParam));//기본 메세지처리..
}

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
	//printf("[%s] %s", msg, lpMsgBuf);
	LocalFree(lpMsgBuf);
}
