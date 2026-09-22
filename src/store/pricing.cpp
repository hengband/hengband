#include "store/pricing.h"
#include "object/object-value.h"
#include "player/player-status-table.h"
#include "store/gold-magnification-table.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include <algorithm>
#include <array>

namespace {
constexpr auto BLACK_MARKET_LEVEL_LIMIT = 500; //!< 闇市の倍率が上がり続けるレベルの上限

/*!
 * @brief 闇市の倍率の表を作る
 * @return レベルごとの倍率 (10000 で等倍)。添字はレベル - 1
 * @details レベル1で2倍とし、レベルが1上がるごとに1.01倍して切り捨てる。
 */
constexpr std::array<uint64_t, BLACK_MARKET_LEVEL_LIMIT> make_black_market_multipliers()
{
    std::array<uint64_t, BLACK_MARKET_LEVEL_LIMIT> multipliers{};
    uint64_t multiplier = 20000;
    for (auto &m : multipliers) {
        m = multiplier;
        multiplier = multiplier * 101 / 100;
    }

    return multipliers;
}

constexpr auto BLACK_MARKET_MULTIPLIERS = make_black_market_multipliers();
static_assert(BLACK_MARKET_MULTIPLIERS[0] == 20000);
static_assert(BLACK_MARKET_MULTIPLIERS[1] == 20200);
}

/*!
 * @brief 闇市で売る品物の価格の倍率を返す
 * @param level 闇市のレベル
 * @return 倍率 (10000 で等倍)。レベル1以下で2倍になり、BLACK_MARKET_LEVEL_LIMIT 以上では変わらない
 */
uint64_t get_black_market_multiplier(int level)
{
    const auto index = std::clamp(level, 1, BLACK_MARKET_LEVEL_LIMIT) - 1;
    return BLACK_MARKET_MULTIPLIERS[index];
}

/*!
 * @brief 店舗価格を計算する
 * @param price アイテムの基本価格
 * @param markup 店主の強欲さと、種族の相性・魅力による補正の和
 * @param black_market_level 闇市ならそのレベル、闇市でなければ nullopt
 * @param flip TRUEならば店主にとっての買取価格、FALSEなら売出価格を計算
 * @return アイテムの店舗価格。基本価格が0以下なら0、それ以外は1以上になる
 * @details
 * markup が 300 のとき補正は 100% になる。買うときの補正は 100% 以上、売るときの補正は
 * 100% 以下にして、店主が損をしないようにする。
 * 闇市では、売るときは半額にし、買うときはレベルに応じた倍率 (2倍以上) を掛ける。
 * 価格が LOW_PRICE_THRESHOLD 以上なら、さらに買うときは1割増し、売るときは1割引きにする。
 */
int calc_store_price(int price, int markup, tl::optional<int> black_market_level, bool flip)
{
    if (price <= 0) {
        return 0;
    }

    if (flip) {
        const auto adjust = std::min(100 + (300 - markup), 100);
        if (black_market_level) {
            price = price / 2;
        }

        price = (price * adjust + 50L) / 100L;
    } else {
        const auto adjust = std::max(100 + (markup - 300), 100);
        uint64_t p = price;
        if (black_market_level) {
            p = p * get_black_market_multiplier(*black_market_level) / 10000UL;
        }
        p = (p * adjust + 50) / 100;
        price = static_cast<int>(std::min<uint64_t>(p, INT32_MAX));
    }

    if (price <= 0) {
        return 1;
    }

    if (price >= LOW_PRICE_THRESHOLD) {
        price += (flip ? -1 : 1) * price / 10;
    }

    return price;
}

/*!
 * @brief 店舗でのアイテム1個の価格を決定する /
 * Determine the price of an item (qty one) in a store.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param price アイテムの基本価格
 * @param store 価格を決める店舗
 * @param flip TRUEならば店主にとっての買取価格、FALSEなら売出価格を計算
 * @return アイテムの店舗価格
 * @details 店主の強欲さ、店主とプレイヤーの種族の相性、プレイヤーの魅力、闇市のレベルを
 * 集めて calc_store_price() で計算する。
 */
int price_item(PlayerType *player_ptr, int price, const Store &store, bool flip)
{
    const auto &owner = store.get_owner();
    const auto markup = owner.inflate + rgold_adj[enum2i(owner.owner_race)][enum2i(player_ptr->prace)] + adj_chr_gold[player_ptr->stat_index[A_CHR]];
    const auto is_black_market = store.get_sale_type() == StoreSaleType::BLACK;
    const auto black_market_level = is_black_market ? tl::make_optional(store_level(StoreSaleType::BLACK)) : tl::nullopt;
    return calc_store_price(price, markup, black_market_level, flip);
}
