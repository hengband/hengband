#pragma once

#include <cstdint>
#include <tl/optional.hpp>

#define LOW_PRICE_THRESHOLD 10L

class PlayerType;
class Store;
uint64_t get_black_market_multiplier(int level);
int calc_store_price(int price, int markup, tl::optional<int> black_market_level, bool flip);
int price_item(PlayerType *player_ptr, int price, const Store &store, bool flip);
