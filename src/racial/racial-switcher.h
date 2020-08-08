#pragma once

#include "system/angband.h"

typedef struct rpi_type rpi_type;
PERCENTAGE racial_chance(player_type *creature_ptr, rpi_type *rpi_ptr);
int check_racial_level(player_type *creature_ptr, rpi_type *rpi_ptr);
bool exe_racial_power(player_type *creature_ptr, const s32b command);
