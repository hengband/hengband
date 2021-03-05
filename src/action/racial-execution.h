﻿#pragma once

#include "system/angband.h"

enum racial_level_check_result {
	RACIAL_SUCCESS = 1,
	RACIAL_FAILURE = -1,
	RACIAL_CANCEL = 0,
};

typedef struct rpi_type rpi_type;
PERCENTAGE racial_chance(player_type *creature_ptr, rpi_type *rpi_ptr);
racial_level_check_result check_racial_level(player_type *creature_ptr, rpi_type *rpi_ptr);
bool exe_racial_power(player_type *creature_ptr, const s32b command);
