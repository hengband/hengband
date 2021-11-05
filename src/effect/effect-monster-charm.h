#pragma once

#include "system/angband.h"

enum process_result;
struct effect_monster_type;
class PlayerType;
process_result effect_monster_charm(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_undead(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_demon(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_animal(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_charm_living(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_domination(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_crusade(PlayerType *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_capture(PlayerType *player_ptr, effect_monster_type *em_ptr);
