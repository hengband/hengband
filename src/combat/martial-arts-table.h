#pragma once

#include "system/angband.h"
#include "util/dice.h"
#include <fmt/base.h>
#include <string_view>

constexpr auto MAX_MA = 17; /*!< 修行僧マーシャルアーツの技数 / Monk martial arts... */
constexpr auto MA_KNEE = 1; /*!< 金的効果ID */
constexpr auto MA_SLOW = 2; /*!< 減速効果ID */

/* For Monk martial arts */
struct martial_arts {
    martial_arts(fmt::format_string<std::string_view> desc_fmt, PLAYER_LEVEL min_lev, int success_chance, const Dice &dice, int special_effect);

    fmt::format_string<std::string_view> desc; /* 打撃の説明の書式 ({} に対象の名前が入る) */
    PLAYER_LEVEL min_level; /* Minimum level to use */
    int chance; /* Chance of 'success' */
    Dice damage_dice; /* Damage dice */
    int effect; /* Special effects */
};

extern const martial_arts ma_blows[MAX_MA];
