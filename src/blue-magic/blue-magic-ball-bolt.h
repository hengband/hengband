#pragma once
/*!
 * @file blue-magic-ball-bolt.h
 * @brief 青魔法のボール/ボルト系呪文ヘッダ
 */

struct bmc_type;
class PlayerType;
bool cast_blue_magic_ball(PlayerType *player_ptr, bmc_type *bmc_ptr);
bool cast_blue_magic_bolt(PlayerType *player_ptr, bmc_type *bmc_ptr);
