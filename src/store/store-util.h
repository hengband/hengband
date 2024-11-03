#pragma once

#include "system/item-entity.h"
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
constexpr auto STORE_SALE_TYPE_LIST = EnumRange(StoreSaleType::GENERAL, StoreSaleType::MAX);

/*!
 * @brief 店舗の情報構造体
 */
struct store_type {
    store_type() = default;
    store_type(const store_type &) = delete;
    store_type(store_type &&) = delete;
    store_type &operator=(const store_type &) = delete;
    store_type &operator=(store_type &&) = delete;

    uint8_t type{}; //!< Store type
    uint8_t owner{}; //!< Owner index
    uint8_t extra{}; //!< Unused for now
    short insult_cur{}; //!< Insult counter
    short good_buy{}; //!< Number of "good" buys (3.0.0で廃止)
    short bad_buy{}; //!< Number of "bad" buys (3.0.0で廃止)
    int store_open{}; //!< Closed until this turn
    int last_visit{}; //!< Last visited on this turn
    std::vector<short> regular{}; //!< Table -- Legal regular item kinds
    std::vector<short> table{}; //!< Table -- Legal item kinds
    short stock_num{}; //!< Stock -- Number of entries
    short stock_size{}; //!< @todo vectorのサイズを取れば良くなったので後ほど削除する.
    std::vector<std::unique_ptr<ItemEntity>> stock{}; //!< Stock -- Actual stock items
};

extern store_type *st_ptr;

class PlayerType;
void store_delete();
std::vector<short> store_same_magic_device_pvals(ItemEntity *j_ptr);
void store_item_increase(short i_idx, int item_num);
void store_item_optimize(short i_idx);
int store_carry(ItemEntity *o_ptr);
bool store_object_similar(const ItemEntity *o_ptr, const ItemEntity *j_ptr);
