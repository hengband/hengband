/*!
 * @brief 店舗 (Store) のテスト
 *
 * 店舗が自分の種類を保持し、その種類と店主の番号から店主の情報を引けることを検証する。
 */

#include "store/store-util.h"

#include "store/store-owners.h"

#include <doctest/doctest.h>

TEST_CASE("Store keeps the sale type it was created with")
{
    for (const auto sale_type : STORE_SALE_TYPE_LIST) {
        CAPTURE(sale_type);
        const Store store(sale_type);
        CHECK(store.get_sale_type() == sale_type);
    }
}

TEST_CASE("Store::get_owner returns the owner of its sale type and owner index")
{
    for (const auto &[sale_type, owner_list] : owners) {
        CAPTURE(sale_type);
        Store store(sale_type);
        for (auto i = 0U; i < owner_list.size(); ++i) {
            CAPTURE(i);
            store.owner = static_cast<uint8_t>(i);
            CHECK(&store.get_owner() == &owner_list[i]);
        }
    }
}
