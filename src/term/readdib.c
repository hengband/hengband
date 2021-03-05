﻿/*!
 * @file readdib.c
 * @brief Windows用ビットマップファイル読み込み処理パッケージ /
 * This package provides a routine to read a DIB file and set up the device dependent version of the image.
 * @date 2014/08/08
 * @author
 * This file has been modified for use with "Angband 2.8.2"
 *
 * COPYRIGHT:
 *
 *   (C) Copyright Microsoft Corp. 1993.  All rights reserved.
 *
 *   You have a royalty-free right to use, modify, reproduce and
 *   distribute the Sample Files (and/or any modified version) in
 *   any way you find useful, provided that you agree that
 *   Microsoft has no warranty obligations or liability for any
 *   Sample Application Files which are modified.
 * @details
 * mind.cとあるが実際には超能力者、練気術師、狂戦士、鏡使い、忍者までの
 * 特殊技能を揃えて実装している。
 */

#include <windows.h>

#include "term/readdib.h"

/*
 * Needed for lcc-win32
 */
#ifndef SEEK_SET
#define SEEK_SET 0
#endif

/*!
 * 一度にファイルから読み込むデータ量 / Number of bytes to be read during each read operation
 */
#define MAXREAD  32768

void global_free(DIBINIT *pInfo, INT_PTR *fh, BOOL unlock_needed);

/*!
 * todo コードが古すぎる。何とかしたい
 * @brief 32KBのデータ読み取りを繰り返すことで、64KB以上のデータを一度に読み取るサブルーチン
 * Private routine to read more than 64K at a time Reads data in steps of 32k till all the data has been read.
 * @param fh ファイルヘッダ
 * @param pv 読み取りポインタ
 * @param ul 読み込むバイト数
 * @return
 * 取得できたデータ量をバイトで返す。0ならば何らかのエラー。
 * Returns number of bytes requested, or zero if something went wrong.
 */
static DWORD PASCAL lread(int fh, void *pv, DWORD ul)
{
	DWORD ulT = ul;
	BYTE *hp = static_cast<BYTE*>(pv);

	while (ul > (DWORD)MAXREAD)
	{
		if (_lread(fh, (LPSTR)hp, (WORD)MAXREAD) != MAXREAD)
				return 0;
		ul -= MAXREAD;
		hp += MAXREAD;
	}
	if (_lread(fh, (LPSTR)hp, (WORD)ul) != ul)
		return 0;
	return ulT;
}


/*!
 * @brief BITMAPINFOHEADERを取得してカラーテーブルを基本としたパレットを作成する。
 * Given a BITMAPINFOHEADER, create a palette based on the color table.
 * @param lpInfo BITMAPINFOHEADERのポインタ
 * @return
 * パレットの参照を返す。NULLならばエラー。
 * Returns the handle of a palette, or zero if something went wrong.
 */
static HPALETTE PASCAL MakeDIBPalette(LPBITMAPINFOHEADER lpInfo)
{
	PLOGPALETTE npPal;
	RGBQUAD *lpRGB;
	HPALETTE hLogPal;
	WORD i;

	/*
	 * since biClrUsed field was filled during the loading of the DIB,
	 * we know it contains the number of colors in the color table.
	 */
	if (lpInfo->biClrUsed)
	{
		npPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
						 (WORD)lpInfo->biClrUsed * sizeof(PALETTEENTRY));
		if (!npPal)
			return(FALSE);

		npPal->palVersion = 0x300;
		npPal->palNumEntries = (WORD)lpInfo->biClrUsed;

		/* get pointer to the color table */
		lpRGB = (RGBQUAD*)((LPSTR)lpInfo + lpInfo->biSize);

		/* copy colors from the color table to the LogPalette structure */
		for (i = 0; i < (WORD)lpInfo->biClrUsed; i++, lpRGB++)
		{
			npPal->palPalEntry[i].peRed = lpRGB->rgbRed;
			npPal->palPalEntry[i].peGreen = lpRGB->rgbGreen;
			npPal->palPalEntry[i].peBlue = lpRGB->rgbBlue;
			npPal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
		}

		hLogPal = CreatePalette((LPLOGPALETTE)npPal);
		LocalFree((HANDLE)npPal);
		return(hLogPal);
	}

	/*
	 * 24-bit DIB with no color table.  return default palette.  Another
	 * option would be to create a 256 color "rainbow" palette to provide
	 * some good color choices.
	 */
	else
	{
		return static_cast<HPALETTE>(GetStockObject(DEFAULT_PALETTE));
	}
}


/*!
 * @brief
 * ビットマップファイルを受け取り、画像のデバイス依存の描画のために使われる共通パレットとビットマップを作成する
 * Given a DIB, create a bitmap and corresponding palette to be used for a
 * device-dependent representation of the image.
 * @param hDC デバイスコンテキストハンドル
 * @param hDIB ビットマップ画像ハンドル
 * @param phPal パレット取得ハンドル
 * @param phBitmap ビットマップ取得ハンドル
 * @return
 * 成功したならばTRUEを返す。失敗の場合FALSE。
 * Returns TRUE on success (phPal and phBitmap are filled with appropriate
 * handles.  Caller is responsible for freeing objects) and FALSE on failure
 * (unable to create objects, both pointer are invalid).
 */
static BOOL PASCAL MakeBitmapAndPalette(HDC hDC, HANDLE hDIB, HPALETTE * phPal, HBITMAP * phBitmap)
{
	LPBITMAPINFOHEADER lpInfo;
	BOOL result = FALSE;
	HBITMAP hBitmap;
	HPALETTE hPalette, hOldPal;
	LPSTR lpBits;

	lpInfo = (LPBITMAPINFOHEADER) GlobalLock(hDIB);
	if ((hPalette = MakeDIBPalette(lpInfo)) != 0)
	{
		/* Need to realize palette for converting DIB to bitmap. */
		hOldPal = SelectPalette(hDC, hPalette, TRUE);
		RealizePalette(hDC);

		lpBits = ((LPSTR)lpInfo + (WORD)lpInfo->biSize +
			  (WORD)lpInfo->biClrUsed * sizeof(RGBQUAD));
		hBitmap = CreateDIBitmap(hDC, lpInfo, CBM_INIT, lpBits,
					 (LPBITMAPINFO)lpInfo, DIB_RGB_COLORS);

		SelectPalette(hDC, hOldPal, TRUE);
		RealizePalette(hDC);

		if (!hBitmap)
		{
			DeleteObject(hPalette);
		}
		else
		{
			*phBitmap = hBitmap;
			*phPal = hPalette;
			result = TRUE;
		}
	}
	return(result);
}



/*!
 * @brief
 * ビットマップファイルを読み込み、BITMAPINFO構造体にハンドルを取得する。
 * Reads a DIB from a file, obtains a handle to its BITMAPINFO struct, and
 * loads the DIB.  Once the DIB is loaded, the function also creates a bitmap
 * and palette out of the DIB for a device-dependent form.
 * device-dependent representation of the image.
 * @param hWnd ウィンドウハンドル
 * @param lpFileName 読み込むビットマップファイル
 * @param pInfo 取得情報を補完するビットマップ情報構造体ポインタ
 * @return
 * Returns TRUE if the DIB is loaded and the bitmap/palette created, in which
 * case, the DIBINIT structure pointed to by pInfo is filled with the appropriate
 * handles, and FALSE if something went wrong.
 * @details
 * Reads a DIB from a file, obtains a handle to its BITMAPINFO struct, and
 * loads the DIB.  Once the DIB is loaded, the function also creates a bitmap
 * and palette out of the DIB for a device-dependent form.
 */
BOOL ReadDIB(HWND hWnd, LPSTR lpFileName, DIBINIT *pInfo)
{
	LPBITMAPINFOHEADER lpbi;
	OFSTRUCT of;
	BITMAPFILEHEADER bf;
	WORD nNumColors;
	BOOL result = FALSE;
	char str[128];
	WORD offBits;
	HDC hDC;
	BOOL bCoreHead = FALSE;

	/* Open the file and get a handle to it's BITMAPINFO */
	INT_PTR fh = OpenFile(lpFileName, &of, OF_READ);
	if (fh == -1)
	{
		wsprintf(str, "Can't open file '%s'", (LPSTR)lpFileName);
		MessageBox(NULL, str, "Error", MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	pInfo->hDIB = GlobalAlloc(GHND, (DWORD)(sizeof(BITMAPINFOHEADER) +
				  256 * sizeof(RGBQUAD)));

	if (!pInfo->hDIB)
		return FALSE;

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(pInfo->hDIB);

	BOOL is_read_error = sizeof(bf) != _lread(fh, (LPSTR)&bf, sizeof(bf));
	is_read_error |= bf.bfType != 0x4d42;
	is_read_error |= sizeof(BITMAPCOREHEADER) != _lread(fh, (LPSTR)lpbi, sizeof(BITMAPCOREHEADER));
	if (is_read_error)
	{
		global_free(pInfo, &fh, TRUE);
		return result;
	}

	if (lpbi->biSize == sizeof(BITMAPCOREHEADER))
	{
		lpbi->biSize = sizeof(BITMAPINFOHEADER);
		lpbi->biBitCount = ((LPBITMAPCOREHEADER)lpbi)->bcBitCount;
		lpbi->biPlanes = ((LPBITMAPCOREHEADER)lpbi)->bcPlanes;
		lpbi->biHeight = ((LPBITMAPCOREHEADER)lpbi)->bcHeight;
		lpbi->biWidth = ((LPBITMAPCOREHEADER)lpbi)->bcWidth;
		bCoreHead = TRUE;
	}
	else
	{
		/* get to the start of the header and read INFOHEADER */
		_llseek(fh, sizeof(BITMAPFILEHEADER), SEEK_SET);
		if (sizeof(BITMAPINFOHEADER) != _lread(fh, (LPSTR)lpbi, sizeof(BITMAPINFOHEADER)))
		{
			global_free(pInfo, &fh, TRUE);
			return result;
		}
	}

	nNumColors = (WORD)lpbi->biClrUsed;
	if (!nNumColors)
	{
		/* no color table for 24-bit, default size otherwise */
		if (lpbi->biBitCount != 24)
			nNumColors = 1 << lpbi->biBitCount;
	}

	/* fill in some default values if they are zero */
	if (lpbi->biClrUsed == 0)
		lpbi->biClrUsed = nNumColors;

	if (lpbi->biSizeImage == 0)
	{
		lpbi->biSizeImage = (((((lpbi->biWidth * (DWORD)lpbi->biBitCount) + 31) & ~31) >> 3)
				     * lpbi->biHeight);
	}

	/* otherwise wouldn't work with 16 color bitmaps -- S.K. */
	else if ((nNumColors == 16) && (lpbi->biSizeImage > bf.bfSize))
	{
		lpbi->biSizeImage /= 2;
	}

	/* get a proper-sized buffer for header, color table and bits */
	GlobalUnlock(pInfo->hDIB);
	pInfo->hDIB = GlobalReAlloc(pInfo->hDIB, lpbi->biSize +
										nNumColors * sizeof(RGBQUAD) +
										lpbi->biSizeImage, 0);

	/* can't resize buffer for loading */
	if (!pInfo->hDIB)
	{
		global_free(pInfo, &fh, FALSE);
		return result;
	}

	lpbi = (LPBITMAPINFOHEADER)GlobalLock(pInfo->hDIB);

	/* read the color table */
	if (!bCoreHead)
	{
		_lread(fh, (LPSTR)(lpbi) + lpbi->biSize, nNumColors * sizeof(RGBQUAD));
	}
	else
	{
		signed int i;
		RGBQUAD *pQuad;
		RGBTRIPLE *pTriple;

		_lread(fh, (LPSTR)(lpbi) + lpbi->biSize, nNumColors * sizeof(RGBTRIPLE));

		pQuad = (RGBQUAD*)((LPSTR)lpbi + lpbi->biSize);
		pTriple = (RGBTRIPLE*) pQuad;
		for (i = nNumColors - 1; i >= 0; i--)
		{
			pQuad[i].rgbRed = pTriple[i].rgbtRed;
			pQuad[i].rgbBlue = pTriple[i].rgbtBlue;
			pQuad[i].rgbGreen = pTriple[i].rgbtGreen;
			pQuad[i].rgbReserved = 0;
		}
	}

	/* offset to the bits from start of DIB header */
	offBits = (WORD)lpbi->biSize + nNumColors * sizeof(RGBQUAD);

	if (bf.bfOffBits != 0L)
	{
		_llseek(fh,bf.bfOffBits,SEEK_SET);
	}

	/* Use local version of '_lread()' above */
	if (lpbi->biSizeImage == lread(fh, (LPSTR)lpbi + offBits, lpbi->biSizeImage))
	{
		GlobalUnlock(pInfo->hDIB);

		hDC = GetDC(hWnd);
		if (!MakeBitmapAndPalette(hDC, pInfo->hDIB, reinterpret_cast<HPALETTE*>(&pInfo->hPalette),
					  reinterpret_cast<HBITMAP*>(&pInfo->hBitmap)))
		{
			ReleaseDC(hWnd,hDC);
			global_free(pInfo, &fh, FALSE);
			return result;
		}
		else
		{
			ReleaseDC(hWnd,hDC);
			result = TRUE;
		}
	}
	else
	{
		GlobalUnlock(pInfo->hDIB);
		GlobalFree(pInfo->hDIB);
	}

	_lclose(fh);
	return result;
}


void global_free(DIBINIT *pInfo, INT_PTR *fh, BOOL unlock_needed)
{
	if (unlock_needed)
	{
		GlobalUnlock(pInfo->hDIB);
	}

	GlobalFree(pInfo->hDIB);
	_lclose(*fh);
}
