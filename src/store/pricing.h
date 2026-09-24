#pragma once

#include <cstdint>
#include <tl/optional.hpp>

#define LOW_PRICE_THRESHOLD 10L

/*!
 * @brief 店舗での取引の向き (プレイヤーから見て)
 */
enum class StoreTradeType {
    PLAYER_BUYS, //!< プレイヤーが店から買う (店の売出価格)
    PLAYER_SELLS, //!< プレイヤーが店に売る (店の買取価格)
};

class PlayerType;
class Store;
uint64_t get_black_market_multiplier(int level);
int calc_store_price(int price, int markup, tl::optional<int> black_market_level, StoreTradeType trade_type);
int price_item(PlayerType *player_ptr, int price, const Store &store, StoreTradeType trade_type);
