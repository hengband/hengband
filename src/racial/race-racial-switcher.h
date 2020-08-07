#pragma once

#include "system/angband.h"

typedef struct rc_type rc_type;
void switch_mimic_racial(player_type *creature_ptr, rc_type *rc_ptr);
void switch_race_racial(player_type *creature_ptr, rc_type *rc_ptr);
