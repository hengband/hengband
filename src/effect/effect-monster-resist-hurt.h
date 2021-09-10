#pragma once

#include "system/angband.h"

struct effect_monster_type;
struct player_type;
process_result effect_monster_nothing(effect_monster_type *em_ptr);
process_result effect_monster_acid(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_elec(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_fire(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_cold(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_pois(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_nuke(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_hell_fire(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_holy_fire(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_plasma(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_nether(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_water(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_chaos(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_shards(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_rocket(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_sound(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_confusion(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_disenchant(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_nexus(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_force(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_inertial(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_time(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_gravity(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_disintegration(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_icee_bolt(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_void(player_type *player_ptr, effect_monster_type *em_ptr);
process_result effect_monster_abyss(player_type *player_ptr, effect_monster_type *em_ptr);
