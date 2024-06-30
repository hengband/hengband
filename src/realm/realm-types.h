#pragma once

#include "util/enum-range.h"

enum magic_realm_type {
    REALM_NONE = 0,
    REALM_LIFE = 1,
    REALM_SORCERY = 2,
    REALM_NATURE = 3,
    REALM_CHAOS = 4,
    REALM_DEATH = 5,
    REALM_TRUMP = 6,
    REALM_ARCANE = 7,
    REALM_CRAFT = 8,
    REALM_DAEMON = 9,
    REALM_CRUSADE = 10,
    MAX_MAGIC = 10,
    MIN_TECHNIC = 16,
    REALM_MUSIC = 16,
    REALM_HISSATSU = 17,
    REALM_HEX = 18,
    REALM_MAX,
};

constexpr auto MAGIC_REALM_RANGE = EnumRangeInclusive(REALM_LIFE, REALM_CRUSADE);
constexpr auto TECHNIC_REALM_RANGE = EnumRangeInclusive(REALM_MUSIC, REALM_HEX);
