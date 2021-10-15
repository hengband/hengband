﻿/*!
 * @file h-define.h
 * @brief 変愚蛮怒で新しく追加された主要なマクロ定義ヘッダ / Define some simple constants
 * @date 2014/08/16
 * @author
 * 不明(変愚蛮怒開発チーム？)
 */

#ifndef INCLUDED_H_DEFINE_H
#define INCLUDED_H_DEFINE_H

/**** Simple "Macros" ****/
#ifdef JP
#define lbtokg(x) ((int)((x)*5)) /*!< 変愚蛮怒基準のポンド→キログラム変換定義(全体) */
#define lbtokg1(x) (lbtokg(x)/100) /*!< 変愚蛮怒基準のポンド→キログラム変換定義(整数部) */
#define lbtokg2(x) ((lbtokg(x)%100)/10)  /*!< 変愚蛮怒基準のポンド→キログラム変換定義(少数部) */
#endif

/*
 * Refer to the member at offset of structure
 */
#define atoffset(TYPE, STRUCT_PTR, OFFSET) (*(TYPE*)(((char*)STRUCT_PTR) + (OFFSET)))

#endif
