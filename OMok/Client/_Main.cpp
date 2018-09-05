#include <Windows.h>
#include <crtdbg.h>//�޸� ���˻�
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
	WndClass.cbWndExtra = 0;//��� 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//�̸��������� �귯�� ������
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;//���ν��� �޾ƿ� �޼��� �����Ǹ� ��Ƴ��� �Լ�
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;//������ ��Ÿ��

	RegisterClass(&WndClass);//os�� ������ ���

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 716 + 16, 538 + 39,
		NULL, (HMENU)NULL, hInstance, NULL);
	//CW_ ->x y wid hei
	//�θ�������, idset, instand, NULL

	ShowWindow(hWnd, nCmdShow);//�ѹ� �׷���


	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return -1;
	}

	client_sock = socket(AF_INET, SOCK_STREAM, 0);//���ӿ� ����
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
		TranslateMessage(&Message);//�޼��� ���� ť���� �����ͼ� ���� �Ļ�ó��
		DispatchMessage(&Message);//������ ���ν����� �� ��
	}

	return (int)Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
//�ڵ�, ��޼���, �ΰ�����1, �ΰ�����2
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
		PostQuitMessage(0);//ť�� �����ٴ¸޼��� ����
		return 0;
		break;
	}

	return(DefWindowProc(hWnd, iMessage, wParam, lParam));//�⺻ �޼���ó��..
}

void err_quit(char *msg)//���� ��� �� ���� //�ɰ��� ���� �߻���
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

void err_display(char *msg)//�������
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
