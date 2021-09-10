#pragma once
/*!
 * @file movement-execution.h
 * @brief プレイヤーの歩行処理実行ヘッダ
 */

#include "system/angband.h"

struct player_type;
void exe_movement(player_type *player_ptr, DIRECTION dir, bool do_pickup, bool break_trap);
