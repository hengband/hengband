#pragma once
/*!
 * @file tunnel-execution.h
 * @brief 掘削処理ヘッダ
 */

#include "system/angband.h"

struct player_type;
bool exe_tunnel(player_type *creature_ptr, POSITION y, POSITION x);
