#pragma once

#include "player-info/race-types.h"
#include "store/store-util.h"
#include "system/angband.h"
#include <unordered_map>
#include <vector>

/*!
 * @brief 店主データ構造体
 */
struct owner_type {
    concptr owner_name; //!< 名前 / Name
    PRICE max_cost; //!< 買取上限 / Purse limit
    byte inflate; //!< 価格上乗せ率 / Inflation
    PlayerRaceType owner_race; //!< 店主種族 / Owner race
    DEPTH level{}; //!< Production Level
};

extern const std::unordered_map<StoreSaleType, std::vector<owner_type>> owners;
