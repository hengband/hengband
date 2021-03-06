#pragma once

#include "system/angband.h"

enum process_result;
typedef struct effect_monster_type effect_monster_type;
process_result effect_monster_psi(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_psi_drain(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_telekinesis(player_type *caster_ptr, effect_monster_type *em_ptr);
