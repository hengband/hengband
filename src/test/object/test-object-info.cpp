/*!
 * @brief 効果発動中に変化したアイテム番号の解決を検証する (#5678)
 */

#include "object/object-info.h"
#include "system/item/item-entity.h"

#include <doctest/doctest.h>
#include <memory>
#include <vector>

TEST_CASE("find_item_index resolves a floor scroll moved by deletion of another item")
{
    const auto dummy = std::make_shared<ItemEntity>();
    const auto neighbor = std::make_shared<ItemEntity>();
    const auto scroll = std::make_shared<ItemEntity>();
    std::vector<std::shared_ptr<ItemEntity>> items{ dummy, neighbor, scroll };
    REQUIRE(find_item_index(items, scroll) == 2);

    // delete_object_idx()は末尾の巻物を削除位置に移し、元の番号を範囲外にする。
    items[1] = items.back();
    items.pop_back();
    const auto current_idx = find_item_index(items, scroll);
    REQUIRE(current_idx.has_value());
    CHECK(*current_idx == 1);
    CHECK(items.at(static_cast<size_t>(*current_idx)) == scroll);
}

TEST_CASE("find_item_index tracks an item across multiple floor compactions")
{
    const auto scroll = std::make_shared<ItemEntity>();
    std::vector<std::shared_ptr<ItemEntity>> items{
        std::make_shared<ItemEntity>(), std::make_shared<ItemEntity>(),
        std::make_shared<ItemEntity>(), std::make_shared<ItemEntity>(), scroll
    };
    items[3] = items.back();
    items.pop_back();
    items[1] = items.back();
    items.pop_back();
    CHECK(find_item_index(items, scroll) == 1);
}

TEST_CASE("find_item_index does not resolve a removed item to its replacement")
{
    const auto scroll = std::make_shared<ItemEntity>();
    const auto replacement = std::make_shared<ItemEntity>();
    std::vector<std::shared_ptr<ItemEntity>> items{ std::make_shared<ItemEntity>(), scroll, replacement };
    items[1] = items.back();
    items.pop_back();
    CHECK_FALSE(find_item_index(items, scroll).has_value());
    CHECK(find_item_index(items, replacement) == 1);
}

TEST_CASE("find_item_index resolves inventory slot zero and reordered items")
{
    const auto scroll = std::make_shared<ItemEntity>();
    const auto other = std::make_shared<ItemEntity>();
    std::vector<std::shared_ptr<ItemEntity>> items{ scroll, other };
    CHECK(find_item_index(items, scroll) == 0);
    items[0] = other;
    items[1] = scroll;
    CHECK(find_item_index(items, scroll) == 1);
}

TEST_CASE("find_item_index rejects null items and empty collections")
{
    const std::vector<std::shared_ptr<ItemEntity>> empty;
    const std::vector<std::shared_ptr<ItemEntity>> null_slot{ nullptr };
    const auto scroll = std::make_shared<ItemEntity>();
    CHECK_FALSE(find_item_index(empty, scroll).has_value());
    CHECK_FALSE(find_item_index(null_slot, nullptr).has_value());
    CHECK_FALSE(find_item_index(null_slot, scroll).has_value());
}
