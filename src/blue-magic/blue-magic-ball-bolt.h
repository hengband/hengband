#pragma once

#include "system/angband.h"

typedef struct bmc_type bmc_type;
bool cast_blue_ball_acid(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_elec(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_fire(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_cold(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_pois(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_nuke(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_nether(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_chaos(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_water(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_star_burst(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_dark_storm(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_mana_storm(player_type *caster_ptr, bmc_type *bmc_ptr);
