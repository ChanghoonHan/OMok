#pragma once
#include <Windows.h>
#include <string>

using namespace std;

class BitMap
{
	HDC m_hdc;
	HBITMAP m_NBitMap;
	HBITMAP m_OBitMap;
	int m_imageWidth;
	int m_imageHeight;

public:
	void Draw(HDC hdc, int x, int y, int width = 26, int height = 26);
	void Init(HDC hdc, string bitMapName);
	void Release();

	BitMap();
	~BitMap();
};

