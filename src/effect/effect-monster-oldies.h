#pragma once

#include "system/angband.h"

typedef enum process_result process_resut;
typedef struct effect_monster_type effect_monster_type;
process_result effect_monster_old_poly(effect_monster_type *em_ptr);
process_result effect_monster_old_clone(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_star_heal(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_heal(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_speed(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_slow(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_sleep(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_conf(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_stasis(effect_monster_type *em_ptr, bool to_evil);
process_result effect_monster_stun(effect_monster_type *em_ptr);
