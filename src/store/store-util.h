﻿#pragma once

#include "system/angband.h"

#include "util/enum-range.h"

#include <memory>
#include <vector>

#define STORE_OBJ_LEVEL 5 //!< 通常店舗の階層レベル / Magic Level for normal stores

enum class StoreSaleType {
    GENERAL   = 0, //!< 店舗の種類: 雑貨屋
    ARMOURY   = 1, //!< 店舗の種類: 防具屋
    WEAPON    = 2, //!< 店舗の種類: 武器屋
    TEMPLE    = 3, //!< 店舗の種類: 寺院
    ALCHEMIST = 4, //!< 店舗の種類: 錬金術の店
    MAGIC     = 5, //!< 店舗の種類: 魔道具屋
    BLACK     = 6, //!< 店舗の種類: ブラック・マーケット
    HOME      = 7, //!< 店舗の種類: 我が家
    BOOK      = 8, //!< 店舗の種類: 書店
    MUSEUM    = 9, //!< 店舗の種類: 博物館
    MAX       = 10
};

constexpr auto STORE_SALE_TYPE_LIST = EnumRange(StoreSaleType::GENERAL, StoreSaleType::MUSEUM);

using store_k_idx = std::vector<KIND_OBJECT_IDX>;

/*!
 * @brief 店舗の情報構造体
 */
struct object_type;
struct store_type {
    byte type{};           //!< Store type
    byte owner{};          //!< Owner index
    byte extra{};          //!< Unused for now
    int16_t insult_cur{};     //!< Insult counter
    int16_t good_buy{};       //!< Number of "good" buys (3.0.0で廃止)
    int16_t bad_buy{};        //!< Number of "bad" buys (3.0.0で廃止)
    int32_t store_open{};     //!< Closed until this turn
    int32_t last_visit{};     //!< Last visited on this turn
    store_k_idx regular{}; //!< Table -- Legal regular item kinds
    store_k_idx table{};   //!< Table -- Legal item kinds
    int16_t stock_num{};      //!< Stock -- Number of entries
    int16_t stock_size{};     //!< Stock -- Total Size of Array
    std::unique_ptr<object_type[]> stock{}; //!< Stock -- Actual stock items

    store_type() = default;
    store_type(const store_type &) = delete;
    store_type &operator=(const store_type &) = delete;
};

extern StoreSaleType cur_store_num;
extern store_type *st_ptr;

class PlayerType;
typedef bool (*black_market_crap_pf)(PlayerType *, object_type *);
typedef bool (*store_will_buy_pf)(PlayerType *, const object_type *);
typedef void (*mass_produce_pf)(PlayerType *, object_type *);
void store_delete(void);
void store_create(PlayerType *player_ptr, KIND_OBJECT_IDX k_idx, black_market_crap_pf black_market_crap, store_will_buy_pf store_will_buy, mass_produce_pf mass_produce);
void store_item_increase(INVENTORY_IDX item, ITEM_NUMBER num);
void store_item_optimize(INVENTORY_IDX item);
int store_carry(object_type *o_ptr);
bool store_object_similar(object_type *o_ptr, object_type *j_ptr);
