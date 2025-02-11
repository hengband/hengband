#pragma once
/*!
 * @file blue-magic-breath.h
 * @brief 青魔法のブレス系呪文ヘッダ
 */

struct bmc_type;
class PlayerType;
bool cast_blue_magic_breath(PlayerType *player_ptr, bmc_type *bmc_ptr);
