#pragma once

/*
 * @file player-ability-types.h
 * @brief アビリティスコア定義 (順番はAD&D初版準拠)
 */

enum player_ability_type : int {
    A_STR = 0,
    A_INT = 1,
    A_WIS = 2,
    A_DEX = 3,
    A_CON = 4,
    A_CHR = 5,
    A_MAX = 6,
    A_RANDOM = 7,
};
