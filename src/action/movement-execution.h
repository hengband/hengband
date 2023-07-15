#pragma once
/*!
 * @file movement-execution.h
 * @brief プレイヤーの歩行処理実行ヘッダ
 */

#include "system/angband.h"

class PlayerType;
void exe_movement(PlayerType *player_ptr, DIRECTION dir, bool do_pickup, bool break_trap);
