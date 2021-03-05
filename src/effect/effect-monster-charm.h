﻿#pragma once

#include "system/angband.h"

enum process_result;
typedef struct effect_monster_type effect_monster_type;
process_result effect_monster_charm(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_undead(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_demon(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_control_animal(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_charm_living(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_domination(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_crusade(player_type *caster_ptr, effect_monster_type *em_ptr);
process_result effect_monster_capture(player_type *caster_ptr, effect_monster_type *em_ptr);
