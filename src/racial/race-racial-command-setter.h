#pragma once

#include "system/angband.h"

typedef struct rc_type rc_type;
void set_mimic_racial_command(player_type *creature_ptr, rc_type *rc_ptr);
void set_race_racial_command(player_type *creature_ptr, rc_type *rc_ptr);
