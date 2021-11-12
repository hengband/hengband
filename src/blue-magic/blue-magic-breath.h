#pragma once
/*!
 * @file blue-magic-breath.h
 * @brief 青魔法のブレス系呪文ヘッダ
 */

struct bmc_type;
class PlayerType;
bool cast_blue_breath_acid(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_elec(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_fire(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_cold(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_pois(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_nether(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_lite(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_dark(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_conf(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_sound(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_chaos(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_disenchant(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_nexus(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_time(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_inertia(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_gravity(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_shards(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_plasma(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_force(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_mana(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_nuke(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_breath_disintegration(PlayerType *player_ptr, bmc_type *bmc_ptr);
