﻿#pragma once
/*!
 * @file open-util.h
 * @brief 開閉処理関連関数ヘッダ
 */

#include "system/angband.h"

struct floor_type;
class player_type;
OBJECT_IDX chest_check(floor_type *floor_ptr, POSITION y, POSITION x, bool trapped);
int count_chests(player_type *player_ptr, POSITION *y, POSITION *x, bool trapped);
