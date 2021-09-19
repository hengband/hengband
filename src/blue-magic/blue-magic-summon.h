#pragma once
/*!
 * @file blue-magic-summon.h
 * @brief 青魔法の召喚系スペルヘッダ
 */

struct bmc_type;
struct player_type;
bool cast_blue_summon_kin(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_cyber(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_monster(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_monsters(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_ant(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_spider(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_hound(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_hydra(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_angel(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_demon(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_undead(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_dragon(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_high_undead(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_high_dragon(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_amberite(player_type *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_summon_unique(player_type *player_ptr, bmc_type *bmc_ptr);
