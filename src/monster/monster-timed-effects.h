#pragma once

#include "util/enum-range.h"
#include <map>
#include <string>

enum class MonsterTimedEffect : int {
    SLEEP = 0,
    FAST = 1,
    SLOW = 2,
    STUN = 3,
    CONFUSION = 4,
    FEAR = 5,
    INVULNERABILITY = 6,
    MAX = 7,
};

constexpr auto MONSTER_TIMED_EFFECT_RANGE = EnumRange<MonsterTimedEffect>(MonsterTimedEffect::SLEEP, MonsterTimedEffect::MAX);

extern const std::map<MonsterTimedEffect, std::string> effect_type_to_label;
