#pragma once

#include "system/angband.h"

struct effect_monster_type;
class PlayerType;
ProcessResult effect_monster_nothing(effect_monster_type *em_ptr);
ProcessResult effect_monster_acid(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_elec(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_fire(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_cold(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_pois(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_nuke(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_hell_fire(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_holy_fire(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_plasma(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_nether(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_water(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_chaos(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_shards(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_rocket(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_sound(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_confusion(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_disenchant(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_nexus(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_force(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_inertial(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_time(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_gravity(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_disintegration(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_icee_bolt(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_void(PlayerType *player_ptr, effect_monster_type *em_ptr);
ProcessResult effect_monster_abyss(PlayerType *player_ptr, effect_monster_type *em_ptr);
