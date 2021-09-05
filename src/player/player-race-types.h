#pragma once

#include "util/enum-converter.h"

/*!
 * @details 既にplayer_raceが存在するので_typeと付けた
 */
enum class player_race_type {
    HUMAN = 0,
    HALF_ELF = 1,
    ELF = 2,
    HOBBIT = 3,
    GNOME = 4,
    DWARF = 5,
    HALF_ORC = 6,
    HALF_TROLL = 7,
    AMBERITE = 8,
    HIGH_ELF = 9,
    BARBARIAN = 10,
    HALF_OGRE = 11,
    HALF_GIANT = 12,
    HALF_TITAN = 13,
    CYCLOPS = 14,
    YEEK = 15,
    KLACKON = 16,
    KOBOLD = 17,
    NIBELUNG = 18,
    DARK_ELF = 19,
    DRACONIAN = 20,
    MIND_FLAYER = 21,
    IMP = 22,
    GOLEM = 23,
    SKELETON = 24,
    ZOMBIE = 25,
    VAMPIRE = 26,
    SPECTRE = 27,
    SPRITE = 28,
    BEASTMAN = 29,
    ENT = 30,
    ARCHON = 31,
    BALROG = 32,
    DUNADAN = 33,
    S_FAIRY = 34,
    KUTAR = 35,
    ANDROID = 36,
    MERFOLK = 37,
    MAX,
};

constexpr auto MAX_RACES = enum2i(player_race_type::MAX);
