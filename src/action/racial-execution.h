#pragma once
/*!
 * @file racial-execution.h
 * @brief レイシャルパワー実行処理ヘッダ
 */

#include "system/angband.h"

enum racial_level_check_result {
	RACIAL_SUCCESS = 1,
	RACIAL_FAILURE = -1,
	RACIAL_CANCEL = 0,
};

struct rpi_type;
class PlayerType;
PERCENTAGE racial_chance(PlayerType *player_ptr, rpi_type *rpi_ptr);
racial_level_check_result check_racial_level(PlayerType *player_ptr, rpi_type *rpi_ptr);
bool exe_racial_power(PlayerType *player_ptr, const int32_t command);
