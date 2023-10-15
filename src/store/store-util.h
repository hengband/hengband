#pragma once

#include "system/angband.h"

#include "util/enum-converter.h"
#include "util/enum-range.h"

#include <memory>
#include <vector>

constexpr DEPTH STORE_OBJ_STD_LEVEL = 5; //!< 通常店舗の標準階層レベル / Magic Level for normal stores

enum class StoreSaleType : int {
    GENERAL = 0, //!< 店舗の種類: 雑貨屋
    ARMOURY = 1, //!< 店舗の種類: 防具屋
    WEAPON = 2, //!< 店舗の種類: 武器屋
    TEMPLE = 3, //!< 店舗の種類: 寺院
    ALCHEMIST = 4, //!< 店舗の種類: 錬金術の店
    MAGIC = 5, //!< 店舗の種類: 魔道具屋
    BLACK = 6, //!< 店舗の種類: ブラック・マーケット
    HOME = 7, //!< 店舗の種類: 我が家
    BOOK = 8, //!< 店舗の種類: 書店
    MUSEUM = 9, //!< 店舗の種類: 博物館
    MAX
};
constexpr int MAX_STORES = enum2i(StoreSaleType::MAX); /*!< 店舗の種類最大数 / Total number of stores (see "store.c", etc) */

constexpr auto STORE_SALE_TYPE_LIST = EnumRange(StoreSaleType::GENERAL, StoreSaleType::MUSEUM);

using store_k_idx = std::vector<short>;

/*!
 * @brief 店舗の情報構造体
 */
class ItemEntity;
struct store_type {
    byte type{}; //!< Store type
    byte owner{}; //!< Owner index
    byte extra{}; //!< Unused for now
    int16_t insult_cur{}; //!< Insult counter
    int16_t good_buy{}; //!< Number of "good" buys (3.0.0で廃止)
    int16_t bad_buy{}; //!< Number of "bad" buys (3.0.0で廃止)
    int32_t store_open{}; //!< Closed until this turn
    int32_t last_visit{}; //!< Last visited on this turn
    store_k_idx regular{}; //!< Table -- Legal regular item kinds
    store_k_idx table{}; //!< Table -- Legal item kinds
    int16_t stock_num{}; //!< Stock -- Number of entries
    int16_t stock_size{}; //!< Stock -- Total Size of Array
    std::unique_ptr<ItemEntity[]> stock; //!< Stock -- Actual stock items

    store_type() = default;
    store_type(const store_type &) = delete;
    store_type &operator=(const store_type &) = delete;
};

extern store_type *st_ptr;

class PlayerType;
void store_delete(void);
std::vector<PARAMETER_VALUE> store_same_magic_device_pvals(ItemEntity *j_ptr);
void store_item_increase(INVENTORY_IDX i_idx, ITEM_NUMBER num);
void store_item_optimize(INVENTORY_IDX i_idx);
int store_carry(ItemEntity *o_ptr);
bool store_object_similar(ItemEntity *o_ptr, ItemEntity *j_ptr);
