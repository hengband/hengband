#pragma once

#include "util/enum-range.h"
#include <map>
#include <string>

enum MonsterTimedEffect : int {
    MTIMED_CSLEEP = 0, /* Monster is sleeping */
    MTIMED_FAST = 1, /* Monster is temporarily fast */
    MTIMED_SLOW = 2, /* Monster is temporarily slow */
    MTIMED_STUNNED = 3, /* Monster is stunned */
    MTIMED_CONFUSED = 4, /* Monster is confused */
    MTIMED_MONFEAR = 5, /* Monster is fearful */
    MTIMED_INVULNER = 6, /* Monster is temporarily invulnerable */
    MAX_MTIMED = 7,
};

constexpr auto MONSTER_TIMED_EFFECT_RANGE = EnumRange<MonsterTimedEffect>(MTIMED_CSLEEP, MAX_MTIMED);

extern const std::map<MonsterTimedEffect, std::string> effect_type_to_label;
