﻿#pragma once
/*!
 * @file blue-magic-ball-bolt.h
 * @brief 青魔法のボール/ボルト系呪文ヘッダ
 */

struct bmc_type;
class player_type;
bool cast_blue_ball_acid(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_elec(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_fire(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_cold(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_pois(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_nuke(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_nether(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_chaos(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_water(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_star_burst(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_dark_storm(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_mana_storm(player_type *player_ptr, bmc_type *bmc_ptr);

bool cast_blue_bolt_acid(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_elec(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_fire(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_cold(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_nether(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_water(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_mana(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_plasma(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_icee(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_missile(player_type *player_ptr, bmc_type *bmc_ptr);
