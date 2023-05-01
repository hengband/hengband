#pragma once

#include "store/store-util.h"
#include "system/angband.h"
#include <vector>

/*
 * A structure describing a town with
 * stores and buildings
 */
struct town_type {
    GAME_TEXT name[32];
    uint32_t seed; /* Seed for RNG */
    std::vector<store_type> store; /* The stores [MAX_STORES] */
    byte numstores;
};

constexpr short VALID_TOWNS = 6; // @details 旧海底都市クエストのマップを除外する. 有効な町に差し替え完了したら不要になるので注意.
constexpr auto SECRET_TOWN = 5; // @details ズルの町番号.

extern std::vector<town_type> towns_info;
