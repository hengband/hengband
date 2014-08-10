/*!
 * @file readdib.h
 * @brief ビットマップファイル読み取り処理のヘッダファイル
 * This package provides a routine to read a DIB file and set up the device dependent version of the image.
 * @date 2014/08/08
 * @author
 * Copyright 1991 Microsoft Corporation. All rights reserved.
 * @details
 * This file has been modified for use with "Angband 2.8.2"
 */

/*!
 * @struct DIBINIT
 * @brief ビットマップファイル情報構造体 / Information about a bitmap
 */
typedef struct {
	HANDLE hDIB;
	HANDLE hBitmap;
	HANDLE hPalette;
	BYTE   CellWidth;
	BYTE   CellHeight;
	BYTE   TileWidth;
	BYTE   TileHeight;
	INT    OffsetX;
	INT    OffsetY;
} DIBINIT;

/* Read a DIB from a file */
BOOL ReadDIB(HWND, LPSTR, DIBINIT *);
