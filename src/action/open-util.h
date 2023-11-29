#pragma once
/*!
 * @file open-util.h
 * @brief 開閉処理関連関数ヘッダ
 */

#include "util/point-2d.h"
#include <utility>

class FloorType;
class PlayerType;
short chest_check(FloorType *floor_ptr, const Pos2D &pos, bool trapped);
std::pair<int, Pos2D> count_chests(PlayerType *player_ptr, bool trapped);
