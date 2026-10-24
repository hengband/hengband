#pragma once

#include "store/store-util.h"
#include "system/angband.h"
#include <string_view>
#include <tl/optional.hpp>

/* Store constants */
constexpr auto STORE_INVEN_MAX = 24; /* Max number of discrete objs in inven */
constexpr auto STORE_TURNOVER = 12; /* Normal shop turnover, per day */
constexpr auto STORE_MIN_KEEP = 6; /* Min slots to "always" keep full */
constexpr auto STORE_MAX_KEEP = 21; /* Max slots to "always" keep full */
constexpr auto STORE_SHUFFLE = 21; /* 1/Chance (per day) of an owner changing */
constexpr auto STORE_TICKS = 1000; /* Number of ticks between turnovers */

class PlayerType;
class StoreScreen;
int16_t store_get_stock_capacity(StoreSaleType sst);
int16_t store_get_stock_limit(const Store &store);
void store_shuffle(size_t town_num, StoreSaleType store_num);
void store_maintenance(PlayerType *player_ptr, size_t town_num, Store &store, int chance);
void store_init(size_t town_num, StoreSaleType store_num);
void store_examine(PlayerType *player_ptr, const StoreScreen &screen);
int store_check_num(const ItemEntity *o_ptr, const Store &store);
int store_level(StoreSaleType store_num);
tl::optional<short> input_stock(std::string_view fmt, int min, int max, StoreSaleType store_num);
