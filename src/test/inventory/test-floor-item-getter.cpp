#include "inventory/floor-item-getter.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-context.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "test/scoped-input-term.h"
#include <doctest/doctest.h>

namespace {
constexpr auto PROMPT = "Which? ";

/*!
 * @brief 指定したスロットに、AllMatchItemTester で選べるアイテムを置く
 * @details 有効なアイテムにするだけで種別は持たないので、種別で選ぶアイテムテスターでは選べない。
 *          名前を表示する処理 (一覧の表示や確認) はゲームデータを要するので、テストでは呼ばない
 */
void put_item(PlayerType &player, int slot)
{
    player.inventory[slot]->bi_id = 1;
}

InputContext item_context(ItemInputDetails details)
{
    return { InputKind::ITEM, PROMPT, details };
}
}

TEST_CASE("get_item_floor publishes the item selection context")
{
    test::ScopedInputTerm term;
    auto &player = term.player();
    put_item(player, 0);
    put_item(player, 2);
    const ItemInputDetails details{ .area = ItemSelectionArea::INVENTORY, .inventory = true, .equipment = false, .floor = false, .first_label = 'a', .last_label = 'c', .force = false, .menu = false };

    SUBCASE("select an item by its label")
    {
        term.expect(item_context(details), "c");
        CHECK(get_item_floor(&player, PROMPT, "", USE_INVEN, AllMatchItemTester()) == 2);
    }

    SUBCASE("cancel")
    {
        term.expect(item_context(details), "\033");
        CHECK_FALSE(get_item_floor(&player, PROMPT, "", USE_INVEN, AllMatchItemTester()));
    }

    SUBCASE("select the Force")
    {
        auto force_details = details;
        force_details.force = true;
        term.expect(item_context(force_details), "w");
        CHECK(get_item_floor(&player, PROMPT, "", USE_INVEN | USE_FORCE, AllMatchItemTester()) == INVEN_FORCE);
    }
}

TEST_CASE("get_item_floor publishes the area switched to")
{
    // 持ち物 (a) から / で装備品に切り替え、体の防具 (h) を選ぶ
    test::ScopedInputTerm term;
    auto &player = term.player();
    put_item(player, 0);
    put_item(player, INVEN_BODY);
    term.expect(item_context({ .area = ItemSelectionArea::INVENTORY, .inventory = true, .equipment = true, .floor = false, .first_label = 'a', .last_label = 'a', .force = false, .menu = false }), "/");
    term.then_expect(item_context({ .area = ItemSelectionArea::EQUIPMENT, .inventory = true, .equipment = true, .floor = false, .first_label = 'h', .last_label = 'h', .force = false, .menu = false }), "h");
    CHECK(get_item_floor(&player, PROMPT, "", USE_INVEN | USE_EQUIP, AllMatchItemTester()) == INVEN_BODY);
}

TEST_CASE("get_item_floor does not wait when no item can be selected")
{
    test::ScopedInputTerm term;

    SUBCASE("nothing is selected")
    {
        CHECK_FALSE(get_item_floor(&term.player(), PROMPT, "", USE_INVEN | USE_EQUIP, AllMatchItemTester()));
    }

    // 練気術を選べるときは、選べるアイテムが無ければ待たずに練気術を選ぶ
    SUBCASE("the Force is selected")
    {
        CHECK(get_item_floor(&term.player(), PROMPT, "", USE_INVEN | USE_EQUIP | USE_FORCE, AllMatchItemTester()) == INVEN_FORCE);
    }
}
