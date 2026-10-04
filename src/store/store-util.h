#pragma once

#include "system/enums/store-sale-type.h"
#include "system/item/item-entity.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include <memory>
#include <tl/optional.hpp>
#include <vector>

struct owner_type;

constexpr auto STORE_OBJ_STD_LEVEL = 5; //!< Magic Level for normal stores
constexpr auto MAX_STORES = enum2i(StoreSaleType::MAX); /*!< Total number of stores (see "store.c", etc) */
constexpr auto STORE_SALE_TYPE_LIST = EnumRange(StoreSaleType::GENERAL, StoreSaleType::MAX);

/*!
 * @brief 店舗の情報構造体
 */
class Store {
public:
    explicit Store(StoreSaleType sale_type);
    Store(const Store &) = delete;
    Store(Store &&) = delete;
    Store &operator=(const Store &) = delete;
    Store &operator=(Store &&) = delete;

    uint8_t owner{}; //!< Owner index
    uint8_t extra{}; //!< Unused for now
    short insult_cur{}; //!< Insult counter
    short good_buy{}; //!< Number of "good" buys
    short bad_buy{}; //!< Number of "bad" buys
    int store_open{}; //!< Closed until this turn
    int last_visit{}; //!< Last visited on this turn
    std::vector<short> regular{}; //!< Table -- Legal regular item kinds
    short stock_num{}; //!< Stock -- Number of entries
    short stock_size{}; //!< @todo vectorのサイズを取れば良くなったので後ほど削除する.
    std::vector<std::unique_ptr<ItemEntity>> stock{}; //!< Stock -- Actual stock items

    void increase_item(short i_idx, int item_num);
    void optimize_item(short i_idx);
    void delete_item();
    std::vector<short> collect_same_magic_device_pvals(ItemEntity &item);
    tl::optional<int> carry(ItemEntity &item);
    StoreSaleType get_sale_type() const;
    const owner_type &get_owner() const;

private:
    StoreSaleType sale_type; //!< 店舗の種類
};
