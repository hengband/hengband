#pragma once

#include "util/enum-range.h"
#include <map>
#include <string>

enum class MonsterTimedEffect : int {
    CSLEEP = 0, /* Monster is sleeping */
    FAST = 1, /* Monster is temporarily fast */
    SLOW = 2, /* Monster is temporarily slow */
    STUNNED = 3, /* Monster is stunned */
    CONFUSED = 4, /* Monster is confused */
    MONFEAR = 5, /* Monster is fearful */
    INVULNER = 6, /* Monster is temporarily invulnerable */
    MAX = 7,
};

constexpr auto MONSTER_TIMED_EFFECT_RANGE = EnumRange<MonsterTimedEffect>(MonsterTimedEffect::CSLEEP, MonsterTimedEffect::MAX);

extern const std::map<MonsterTimedEffect, std::string> effect_type_to_label;
