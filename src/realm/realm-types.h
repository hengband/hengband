#pragma once

#include "util/enum-converter.h"
#include "util/enum-range.h"

enum class RealmType {
    NONE = 0,
    LIFE = 1,
    SORCERY = 2,
    NATURE = 3,
    CHAOS = 4,
    DEATH = 5,
    TRUMP = 6,
    ARCANE = 7,
    CRAFT = 8,
    DAEMON = 9,
    CRUSADE = 10,
    MUSIC = 16,
    HISSATSU = 17,
    HEX = 18,
    MAX,
};

constexpr auto MAGIC_REALM_RANGE = EnumRangeInclusive(RealmType::LIFE, RealmType::CRUSADE);
constexpr auto TECHNIC_REALM_RANGE = EnumRangeInclusive(RealmType::MUSIC, RealmType::HEX);
constexpr auto MAX_MAGIC = std::ssize(MAGIC_REALM_RANGE);
constexpr auto MIN_TECHNIC = enum2i(RealmType::MUSIC);
