#pragma once

#include "system/angband.h"

typedef enum process_result process_resut;
typedef struct effect_monster_type effect_monster_type;
process_result switch_effects_monster(player_type *caster_ptr, effect_monster_type *em_ptr);
