// bmp.c
//
// part of checkerboard
//
// this file loads bitmaps for pieces and board
// can read .bmp files and returns HBITMAPs - handles to bitmaps.
//
// when using this from another file, you basically need two functions:
// int initbmp(HWND hwnd, char dir[256]); which is called to start with, and loads bitmaps
// from a directory and
// HBITMAP getCBbitmap(int type); which returns a handle to a bitmap.
// in the other file, you use HBITMAP bitmap;
// and bitmap = getCBbitmap(BMPBLACKMAN);
// include headers
#include <windows.h>
#include <stdio.h>
#include "standardheader.h"
#include "bmp.h"
#include "utility.h"

// function prototypes
BITMAPFILEHEADER *DibLoadImage(PTSTR pstrFileName);
HBITMAP CreateBitmapObjectFromDibFile(HDC hdc, PTSTR szFileName);

// file-global bitmaps for checkers and background.
static HBITMAP bmp_bm, bmp_bk, bmp_wm, bmp_wk, bmp_light, bmp_dark, bmp_manmask, bmp_kingmask;

HBITMAP getCBbitmap(int type)
{
	switch (type) {
	case BMPBLACKMAN:
		return bmp_bm;
		break;

	case BMPBLACKKING:
		return bmp_bk;
		break;

	case BMPWHITEMAN:
		return bmp_wm;
		break;

	case BMPWHITEKING:
		return bmp_wk;
		break;

	case BMPLIGHT:
		return bmp_light;
		break;

	case BMPDARK:
		return bmp_dark;
		break;

	case BMPMANMASK:
		return bmp_manmask;
		break;

	case BMPKINGMASK:
		return bmp_kingmask;
		break;

	default:
		return NULL;
	}
}

int initbmp(HWND hwnd, char dir[256])
{
	static HDC hdc;
	char filename[256];
	int i;

	/* The previous piece set, freed after the new one is loaded. The caller makes sure
	 * that none of them is selected into a DC (see change_pieceset() in graphics.c).
	 */
	HBITMAP old[] = {bmp_manmask, bmp_kingmask, bmp_bm, bmp_bk, bmp_wm, bmp_wk, bmp_light, bmp_dark};

	// get device context
	hdc = GetDC(hwnd);

	// load bitmaps.
	sprintf(filename, "%s\\manmask.bmp", dir);
	bmp_manmask = CreateBitmapObjectFromDibFile(hdc, filename);
	if (bmp_manmask == NULL)
		cblog("could not load bitmap %s\n", filename);

	sprintf(filename, "%s\\kingmask.bmp", dir);
	bmp_kingmask = CreateBitmapObjectFromDibFile(hdc, filename);
	if (bmp_kingmask == NULL)
		cblog("could not load bitmap %s\n", filename);

	sprintf(filename, "%s\\bm.bmp", dir);
	bmp_bm = CreateBitmapObjectFromDibFile(hdc, filename);
	if (bmp_bm == NULL)
		cblog("could not load bitmap %s\n", filename);

	sprintf(filename, "%s\\bk.bmp", dir);
	bmp_bk = CreateBitmapObjectFromDibFile(hdc, filename);
	if (bmp_bk == NULL)
		cblog("could not load bitmap %s\n", filename);

	sprintf(filename, "%s\\wm.bmp", dir);
	bmp_wm = CreateBitmapObjectFromDibFile(hdc, filename);
	if (bmp_wm == NULL)
		cblog("could not load bitmap %s\n", filename);

	sprintf(filename, "%s\\wk.bmp", dir);
	bmp_wk = CreateBitmapObjectFromDibFile(hdc, filename);
	if (bmp_wk == NULL)
		cblog("could not load bitmap %s\n", filename);

	sprintf(filename, "%s\\light.bmp", dir);
	bmp_light = CreateBitmapObjectFromDibFile(hdc, filename);
	if (bmp_light == NULL)
		cblog("could not load bitmap %s\n", filename);

	sprintf(filename, "%s\\dark.bmp", dir);
	bmp_dark = CreateBitmapObjectFromDibFile(hdc, filename);
	if (bmp_dark == NULL)
		cblog("could not load bitmap %s\n", filename);

	ReleaseDC(hwnd, hdc);

	for (i = 0; i < (int)(sizeof(old) / sizeof(old[0])); ++i)
		if (old[i] != NULL)
			DeleteObject(old[i]);

	return 0;
}

BITMAPFILEHEADER *DibLoadImage(PTSTR pstrFileName)
{
	BOOL bSuccess;
	DWORD dwFileSize, dwHighSize, dwBytesRead;
	HANDLE hFile;
	BITMAPFILEHEADER *pbmfh;
	hFile = CreateFile(pstrFileName,
					   GENERIC_READ,
					   FILE_SHARE_READ,
					   NULL,
					   OPEN_EXISTING,
					   FILE_FLAG_SEQUENTIAL_SCAN,
					   NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	dwFileSize = GetFileSize(hFile, &dwHighSize);
	if (dwHighSize) {
		CloseHandle(hFile);
		return NULL;
	}

	pbmfh = (BITMAPFILEHEADER *)malloc(dwFileSize);
	if (!pbmfh) {
		CloseHandle(hFile);
		return NULL;
	}

	bSuccess = ReadFile(hFile, pbmfh, dwFileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);
	if (!bSuccess || (dwBytesRead != dwFileSize) || (pbmfh->bfType != * (WORD *)"BM") || (pbmfh->bfSize != dwFileSize)) {
		free(pbmfh);
		return NULL;
	}

	return pbmfh;
}

HBITMAP CreateBitmapObjectFromDibFile(HDC hdc, PTSTR szFileName)
{
	BITMAPFILEHEADER *pbmfh;
	BOOL bSuccess;
	DWORD dwFileSize, dwHighSize, dwBytesRead;
	HANDLE hFile;
	HBITMAP hBitmap;

	hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return NULL;
	dwFileSize = GetFileSize(hFile, &dwHighSize);
	if (dwHighSize) {
		CloseHandle(hFile);
		return NULL;
	}

	pbmfh = (BITMAPFILEHEADER *)malloc(dwFileSize);

	if (!pbmfh) {
		CloseHandle(hFile);
		return NULL;
	}

	bSuccess = ReadFile(hFile, pbmfh, dwFileSize, &dwBytesRead, NULL);
	CloseHandle(hFile);
	if (!bSuccess || (dwBytesRead != dwFileSize) || (pbmfh->bfType != * (WORD *)"BM") || (pbmfh->bfSize != dwFileSize)) {
		free(pbmfh);
		return NULL;
	}

	hBitmap = CreateDIBitmap(hdc,
							 (BITMAPINFOHEADER *) (pbmfh + 1),
							 CBM_INIT,
							 (BYTE *)pbmfh + pbmfh->bfOffBits,
							 (BITMAPINFO *) (pbmfh + 1),
							 DIB_RGB_COLORS);
	free(pbmfh);
	return hBitmap;
}
