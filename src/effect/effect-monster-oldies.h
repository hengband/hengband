#pragma once

#include "system/angband.h"

struct effect_monster_type;
class PlayerType;
process_result effect_monster_old_poly(effect_monster_type *em_ptr);
process_result effect_monster_old_clone(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_star_heal(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_heal(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_speed(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_slow(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_sleep(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_old_conf(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_stasis(effect_monster_type *em_ptr, bool to_evil);
process_result effect_monster_stun(effect_monster_type *em_ptr);
