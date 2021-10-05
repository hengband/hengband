#pragma once

#include "player-info/race-types.h"
#include "system/angband.h"
#include <array>

#define MAX_STORES 10 /*!< 店舗の種類最大数 / Total number of stores (see "store.c", etc) */
#define MAX_OWNERS 32 /*!< 各店舗毎の店主定義最大数 / Total number of owners per store (see "store.c", etc) */

/*!
 * @brief 店主データ構造体
 */
struct owner_type {
    concptr owner_name; //!< 名前 / Name
    PRICE max_cost; //!< 買取上限 / Purse limit
    byte inflate; //!< 価格上乗せ率 / Inflation
    PlayerRaceType owner_race; //!< 店主種族 / Owner race
};

extern const owner_type owners[MAX_STORES][MAX_OWNERS];
