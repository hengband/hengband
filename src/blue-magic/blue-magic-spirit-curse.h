#pragma once
/*!
 * @file blue-magic-spirit-curse.h
 * @brief 青魔法の呪い系処理ヘッダ
 */

struct bmc_type;
struct player_type;
bool cast_blue_drain_mana(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_mind_blast(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_brain_smash(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_curse_1(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_curse_2(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_curse_3(player_type *caster_ptr, bmc_type *bmc_ptr);
bool cast_blue_curse_4(player_type *caster_ptr, bmc_type *bmc_ptr);
