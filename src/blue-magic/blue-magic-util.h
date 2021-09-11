#pragma once
/*!
 * @file blue-magic-util.h
 * @brief 青魔法の構造体、初期化処理ヘッダ
 */

#include "system/angband.h"

// Blue Magic Cast.
typedef struct bmc_type {
    DIRECTION dir;
    PLAYER_LEVEL plev;
    PLAYER_LEVEL summon_lev;
    HIT_POINT damage;
    bool pet;
    bool no_trump;
    BIT_FLAGS p_mode;
    BIT_FLAGS u_mode;
    BIT_FLAGS g_mode;
} bmc_type;

struct player_type;
typedef PLAYER_LEVEL (*get_pseudo_monstetr_level_pf)(player_type *player_ptr);
bmc_type *initialize_blue_magic_type(
    player_type *player_ptr, bmc_type *bmc_ptr, const bool success, get_pseudo_monstetr_level_pf get_pseudo_monstetr_level);
