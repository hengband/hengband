/*!
 * @brief 店舗の処理 (store.cpp) のテスト
 *
 * 店舗の種類とオプション powerup_home、確保した容量から決まる、品を置ける数の上限を検証する。
 */

#include "store/store.h"

#include "game-option/birth-options.h"
#include "store/store-util.h"
#include "test/scoped-restore.h"

#include <doctest/doctest.h>

TEST_CASE("store_get_stock_limit returns the stock limit of each sale type, and STORE_INVEN_MAX for the home without powerup_home")
{
    const auto restore = test::scoped_restore(powerup_home);
    Store home(StoreSaleType::HOME);
    Store museum(StoreSaleType::MUSEUM);
    Store black(StoreSaleType::BLACK);
    Store general(StoreSaleType::GENERAL);
    for (auto *store : { &home, &museum, &black, &general }) {
        store->stock_size = store_get_stock_capacity(store->get_sale_type());
    }

    powerup_home = false;
    CHECK(store_get_stock_limit(home) == STORE_INVEN_MAX);
    CHECK(store_get_stock_limit(museum) == STORE_INVEN_MAX * 50);
    CHECK(store_get_stock_limit(black) == STORE_INVEN_MAX * 50);
    CHECK(store_get_stock_limit(general) == STORE_INVEN_MAX * 3 / 2);

    powerup_home = true;
    CHECK(store_get_stock_limit(home) == STORE_INVEN_MAX * 10);
    CHECK(store_get_stock_limit(museum) == STORE_INVEN_MAX * 50);
    CHECK(store_get_stock_limit(black) == STORE_INVEN_MAX * 50);
    CHECK(store_get_stock_limit(general) == STORE_INVEN_MAX * 3 / 2);
}

TEST_CASE("store_get_stock_limit returns 0 for a store without allocated stock")
{
    const auto restore = test::scoped_restore(powerup_home);
    const Store home(StoreSaleType::HOME);
    const Store museum(StoreSaleType::MUSEUM);

    powerup_home = false;
    CHECK(store_get_stock_limit(home) == 0);
    CHECK(store_get_stock_limit(museum) == 0);

    powerup_home = true;
    CHECK(store_get_stock_limit(home) == 0);
    CHECK(store_get_stock_limit(museum) == 0);
}
