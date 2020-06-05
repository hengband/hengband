#pragma once

#include "system/angband.h"
#include "monster-attack/monster-attack-util.h"

void check_fall_off_horse(player_type *creature_ptr, monap_type *monap_ptr);
bool process_fall_off_horse(player_type *creature_ptr, HIT_POINT dam, bool force);
