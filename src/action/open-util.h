#pragma once
/*!
 * @file open-util.h
 * @brief 開閉処理関連関数ヘッダ
 */

#include "floor/geometry.h"
#include "util/point-2d.h"
#include <utility>

class FloorType;
class PlayerType;
short chest_check(const FloorType &floor, const Pos2D &pos, bool trapped);
std::pair<int, Direction> count_chests(PlayerType *player_ptr, bool trapped);
