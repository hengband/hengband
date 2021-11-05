#pragma once
/*!
 * @file blue-magic-spirit-curse.h
 * @brief 青魔法の呪い系処理ヘッダ
 */

struct bmc_type;
class PlayerType;
bool cast_blue_drain_mana(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_mind_blast(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_brain_smash(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_curse_1(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_curse_2(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_curse_3(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_curse_4(PlayerType *player_ptr, bmc_type *bmc_ptr);
