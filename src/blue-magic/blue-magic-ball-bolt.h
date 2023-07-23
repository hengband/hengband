#pragma once
/*!
 * @file blue-magic-ball-bolt.h
 * @brief 青魔法のボール/ボルト系呪文ヘッダ
 */

struct bmc_type;
class PlayerType;
bool cast_blue_ball_acid(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_elec(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_fire(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_cold(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_pois(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_nuke(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_nether(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_chaos(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_water(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_star_burst(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_dark_storm(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_mana_storm(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_void(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_ball_abyss(PlayerType *player_ptr, bmc_type *bmc_ptr);

bool cast_blue_bolt_acid(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_elec(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_fire(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_cold(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_nether(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_water(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_mana(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_plasma(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_icee(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_void(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_abyss(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_bolt_missile(PlayerType *player_ptr, bmc_type *bmc_ptr);
