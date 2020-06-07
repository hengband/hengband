#pragma once

#include "system/angband.h"
#include "effect/effect-monster-util.h"

gf_switch_result effect_monster_charm(player_type *caster_ptr, effect_monster_type *em_ptr);
gf_switch_result effect_monster_control_undead(player_type *caster_ptr, effect_monster_type *em_ptr);
gf_switch_result effect_monster_control_demon(player_type *caster_ptr, effect_monster_type *em_ptr);
gf_switch_result effect_monster_control_animal(player_type *caster_ptr, effect_monster_type *em_ptr);
gf_switch_result effect_monster_charm_living(player_type *caster_ptr, effect_monster_type *em_ptr);
