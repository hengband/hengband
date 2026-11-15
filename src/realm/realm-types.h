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

/*!
 * @brief プレイヤーが選んだ領域の位置 (第一領域か第二領域か)
 * @details 職業別の魔法の値は位置ごとに別の表を持つ。全領域職 (スペルマスター・赤魔道師) と技能領域は第一領域の表を使う。
 */
enum class RealmPosition {
    FIRST = 0,
    SECOND = 1,
};
