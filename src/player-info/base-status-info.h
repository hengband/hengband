#pragma once

#include "system/angband.h"

typedef struct self_info_type self_info_type;
void set_equipment_influence(player_type *creature_ptr, self_info_type *si_ptr);
void set_status_sustain_info(player_type *creature_ptr, self_info_type *si_ptr);
