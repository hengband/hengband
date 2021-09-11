#pragma once

#include "system/angband.h"

enum process_result;
struct effect_monster_type;
struct player_type;
process_result effect_monster_charm(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_undead(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_demon(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_animal(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_charm_living(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_domination(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_crusade(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_capture(player_type *player_ptr, effect_monster_type *em_ptr);
