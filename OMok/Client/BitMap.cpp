#include "BitMap.h"

void BitMap::Init(HDC hdc, string bitMapName)
{	
	m_hdc = CreateCompatibleDC(hdc);
	m_NBitMap = (HBITMAP)LoadImage(NULL, bitMapName.c_str(), IMAGE_BITMAP, 0, 0,
		LR_CREATEDIBSECTION | LR_DEFAULTSIZE | LR_LOADFROMFILE);
	BITMAP bmp_info;
	GetObject(m_NBitMap, sizeof(BITMAP), &bmp_info);
	m_imageHeight = bmp_info.bmHeight;
	m_imageWidth = bmp_info.bmWidth;

	m_OBitMap = (HBITMAP)SelectObject(m_hdc, m_NBitMap);
}

void BitMap::Draw(HDC hdc, int x, int y, int width, int height)
{
	TransparentBlt(hdc, x, y, m_imageWidth, m_imageHeight, m_hdc, 0, 0, m_imageWidth, m_imageHeight, RGB(255, 0, 255));
}

void BitMap::Release()
{
	(HBITMAP)SelectObject(m_hdc, m_OBitMap);
	DeleteObject(m_NBitMap);
	DeleteDC(m_hdc);
	m_NBitMap = NULL;
}

BitMap::BitMap()
{
	m_NBitMap = NULL;
}

BitMap::~BitMap()
{
	Release();
}
