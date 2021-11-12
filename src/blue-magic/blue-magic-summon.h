#pragma once
/*!
 * @file blue-magic-summon.h
 * @brief 青魔法の召喚系スペルヘッダ
 */

struct bmc_type;
class PlayerType;
bool cast_blue_summon_kin(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_cyber(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_monster(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_monsters(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_ant(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_spider(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_hound(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_hydra(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_angel(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_demon(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_undead(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_dragon(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_high_undead(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_high_dragon(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_amberite(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_unique(PlayerType *player_ptr, bmc_type *bmc_ptr);
