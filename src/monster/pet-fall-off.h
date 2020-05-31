#pragma once

#include "system/angband.h"
#include "combat/monster-attack-util.h"

void check_fall_off_horse(player_type *target_ptr, monap_type *monap_ptr);
bool process_fall_off_horse(player_type *creature_ptr, HIT_POINT dam, bool force);
