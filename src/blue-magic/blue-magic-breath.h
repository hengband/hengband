﻿#pragma once
/*!
 * @file blue-magic-breath.h
 * @brief 青魔法のブレス系呪文ヘッダ
 */


#include "system/angband.h"

typedef struct bmc_type bmc_type;
bool cast_blue_breath_acid(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_elec(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_fire(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_cold(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_pois(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_nether(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_lite(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_dark(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_conf(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_sound(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_chaos(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_disenchant(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_nexus(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_time(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_inertia(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_gravity(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_shards(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_plasma(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_force(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_mana(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_nuke(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_disintegration(player_type *caster_ptr, bmc_type *bmc_ptr);
