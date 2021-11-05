#pragma once
/*!
 * @file blue-magic-status.h
 * @brief 青魔法の状態異常系スペルヘッダ
 */

struct bmc_type;
class PlayerType;
bool cast_blue_scare(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_blind(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_confusion(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_slow(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_sleep(PlayerType *player_ptr, bmc_type *bmc_ptr);
