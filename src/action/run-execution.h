#pragma once
/*!
 * @file run-execution.h
 * @brief プレイヤーの走行処理ヘッダ
 */

#include "system/angband.h"

extern bool ignore_avoid_run;

class PlayerType;
class Direction;
void run_step(PlayerType *player_ptr, const Direction &dir);
