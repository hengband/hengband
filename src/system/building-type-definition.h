#pragma once

#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "realm/realm-types.h"
#include <array>
#include <vector>

constexpr auto MAX_BUILDINGS = 32; /*!< 施設の種類最大数 / Number of buildings */

enum class MonsterRaceId : short;

struct building_type {
    char name[20]{}; /* proprietor name */
    char owner_name[20]{}; /* proprietor name */
    char owner_race[20]{}; /* proprietor race */

    char act_names[8][30]{}; /* action names */
    int member_costs[8]{}; /* Costs for class members of building */
    int other_costs[8]{}; /* Costs for nonguild members */
    char letters[8]{}; /* action letters */
    short actions[8]{}; /*!< 町の施設処理における行動ID */
    short action_restr[8]{}; /*!< 町の施設処理の規制処理ID */

    std::vector<short> member_class{}; /* which classes are part of guild */
    std::vector<short> member_race{}; /* which races are part of guild */
    std::vector<short> member_realm{}; /* 店主ごとの魔法領域 / which realms are part of guild */
};

extern std::array<building_type, MAX_BUILDINGS> buildings;
extern MonsterRaceId battle_mon_list[4];
