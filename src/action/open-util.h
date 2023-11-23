#pragma once
/*!
 * @file open-util.h
 * @brief 開閉処理関連関数ヘッダ
 */

#include "util/point-2d.h"

class FloorType;
class PlayerType;
short chest_check(FloorType *floor_ptr, const Pos2D &pos, bool trapped);
int count_chests(PlayerType *player_ptr, POSITION *y, POSITION *x, bool trapped);
