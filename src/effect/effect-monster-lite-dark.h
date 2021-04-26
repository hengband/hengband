#pragma once

#include "system/angband.h"

typedef struct effect_monster_type effect_monster_type;
typedef struct player_type player_type;
process_result effect_monster_lite_weak(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_lite(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_dark(player_type *caster_ptr, effect_monster_type *em_ptr);
