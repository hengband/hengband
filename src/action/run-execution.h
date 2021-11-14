#pragma once
/*!
 * @file run-execution.h
 * @brief プレイヤーの走行処理ヘッダ
 */

#include "system/angband.h"

#define MAX_RUN_CYCLES 17
#define MAX_RUN_CHOME 10

extern bool ignore_avoid_run;
extern byte cycle[MAX_RUN_CYCLES];
extern byte chome[MAX_RUN_CHOME];

class PlayerType;
void run_step(PlayerType *player_ptr, DIRECTION dir);
