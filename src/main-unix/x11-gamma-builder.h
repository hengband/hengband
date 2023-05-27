#pragma once
/*!
 * @file x11-gamma-builder.h
 * @brief X11環境ガンマ値の調整処理ヘッダ
 */

#include "system/angband.h"

extern byte gamma_table[256];
void build_gamma_table(int gamma);
