#pragma once

#include "system/angband.h"

typedef enum process_result process_resut;
typedef struct effect_monster_type effect_monster_type;
process_result effect_monster_away_undead(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_away_evil(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_away_all(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_turn_undead(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_turn_evil(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_turn_all(effect_monster_type *em_ptr);
process_result effect_monster_disp_undead(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_disp_evil(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_disp_good(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_disp_living(effect_monster_type *em_ptr);
process_result effect_monster_disp_demon(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_disp_all(effect_monster_type *em_ptr);
