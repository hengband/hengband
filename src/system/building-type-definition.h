#pragma once

#include "player-info/class-types.h"
#include "player-info/race-types.h"
#include "realm/realm-types.h"
#include "system/angband.h"
#include <vector>

#define MAX_BLDG 32 /*!< 施設の種類最大数 / Number of buildings */

enum class MonsterRaceId : int16_t;

struct building_type {
    GAME_TEXT name[20]; /* proprietor name */
    GAME_TEXT owner_name[20]; /* proprietor name */
    GAME_TEXT owner_race[20]; /* proprietor race */

    GAME_TEXT act_names[8][30]; /* action names */
    PRICE member_costs[8]; /* Costs for class members of building */
    PRICE other_costs[8]; /* Costs for nonguild members */
    char letters[8]; /* action letters */
    int16_t actions[8]; /*!< 町の施設処理における行動ID */
    int16_t action_restr[8]; /*!< 町の施設処理の規制処理ID */

    std::vector<short> member_class; /* which classes are part of guild */
    std::vector<short> member_race; /* which races are part of guild */
    std::vector<short> member_realm; /* 店主ごとの魔法領域 / which realms are part of guild */
};

extern building_type building[MAX_BLDG];
extern MonsterRaceId battle_mon_list[4];
