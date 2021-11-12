#pragma once
/*!
 * @file tunnel-execution.h
 * @brief 掘削処理ヘッダ
 */

#include "system/angband.h"

class PlayerType;
bool exe_tunnel(PlayerType *player_ptr, POSITION y, POSITION x);
