/*!
 * @brief RedrawingFlagsUpdater のテスト
 */

#include "system/redrawing-flags-updater.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include "util/flag-group.h"
#include <doctest/doctest.h>
#include <initializer_list>

namespace {

/*!
 * @brief フラグの今の状態を覚えておき、スコープを抜けるときに元の状態に戻す
 * @tparam Flag フラグの列挙型。1 回の呼び出しで戻せるのは同じ列挙型のフラグだけ
 * @param rfu 更新フラグ
 * @param flags 戻すフラグ
 * @return スコープを抜けるときにフラグを戻すファイナライザ
 */
template <typename Flag>
auto scoped_restore_flags(RedrawingFlagsUpdater &rfu, std::initializer_list<Flag> flags)
{
    const EnumClassFlagGroup<Flag> targets(flags);
    EnumClassFlagGroup<Flag> was_set;
    for (const auto flag : flags) {
        if (rfu.has(flag)) {
            was_set.set(flag);
        }
    }

    return util::make_finalizer([&rfu, targets, was_set] {
        rfu.reset_flags(targets);
        rfu.set_flags(was_set);
    });
}

}

TEST_CASE("RedrawingFlagsUpdater::request_inventory_arrangement requests combining, reordering and redrawing the inventory")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_iaf = {
        InventoryArrangementFlag::AUTO_DESTRUCTION,
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    const auto restore_iaf = scoped_restore_flags(rfu, flags_iaf);
    const auto restore_swrf = scoped_restore_flags(rfu, { SubWindowRedrawingFlag::INVENTORY });
    rfu.reset_flags(flags_iaf);
    rfu.reset_flag(SubWindowRedrawingFlag::INVENTORY);

    rfu.request_inventory_arrangement();

    CHECK(rfu.has(InventoryArrangementFlag::COMBINATION));
    CHECK(rfu.has(InventoryArrangementFlag::REORDER));
    CHECK(rfu.has(SubWindowRedrawingFlag::INVENTORY));
    CHECK_FALSE(rfu.has(InventoryArrangementFlag::AUTO_DESTRUCTION));
}

TEST_CASE("RedrawingFlagsUpdater::request_whole_main_window_redrawing requests wiping and redrawing the whole main window")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    const auto restore = scoped_restore_flags(rfu, flags_mwrf);
    rfu.reset_flags(flags_mwrf);

    rfu.request_whole_main_window_redrawing();

    for (const auto flag : flags_mwrf) {
        CAPTURE(enum2i(flag));
        CHECK(rfu.has(flag));
    }
}

TEST_CASE("RedrawingFlagsUpdater::request_whole_status_recalculation requests recalculating the status, the view and the lights for redrawing the whole screen")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    const auto restore = scoped_restore_flags(rfu, flags_srf);
    const auto restore_flow = scoped_restore_flags(rfu, { StatusRecalculatingFlag::FLOW, StatusRecalculatingFlag::DISTANCE });
    rfu.reset_flags(flags_srf);
    rfu.reset_flag(StatusRecalculatingFlag::FLOW);
    rfu.reset_flag(StatusRecalculatingFlag::DISTANCE);

    rfu.request_whole_status_recalculation();

    for (const auto flag : flags_srf) {
        CAPTURE(enum2i(flag));
        CHECK(rfu.has(flag));
    }

    // モンスターの経路と距離は、フロアに入ったときなどに別に計算する
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::FLOW));
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::DISTANCE));
}

TEST_CASE("RedrawingFlagsUpdater::request_map_redrawing requests redrawing the maps")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    const auto restore_mwrf = scoped_restore_flags(rfu, { MainWindowRedrawingFlag::MAP });
    const auto restore_swrf = scoped_restore_flags(rfu, flags_swrf);
    rfu.reset_flag(MainWindowRedrawingFlag::MAP);
    rfu.reset_flags(flags_swrf);

    rfu.request_map_redrawing();

    CHECK(rfu.has(MainWindowRedrawingFlag::MAP));
    CHECK(rfu.has(SubWindowRedrawingFlag::OVERHEAD));
    CHECK(rfu.has(SubWindowRedrawingFlag::DUNGEON));
}

TEST_CASE("RedrawingFlagsUpdater::request_monster_update_and_map_redrawing requests updating the monsters and redrawing the maps")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    const auto restore_srf = scoped_restore_flags(rfu, { StatusRecalculatingFlag::MONSTER_STATUSES });
    const auto restore_mwrf = scoped_restore_flags(rfu, { MainWindowRedrawingFlag::MAP });
    const auto restore_swrf = scoped_restore_flags(rfu, flags_swrf);
    rfu.reset_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.reset_flag(MainWindowRedrawingFlag::MAP);
    rfu.reset_flags(flags_swrf);

    rfu.request_monster_update_and_map_redrawing();

    CHECK(rfu.has(StatusRecalculatingFlag::MONSTER_STATUSES));
    CHECK(rfu.has(MainWindowRedrawingFlag::MAP));
    CHECK(rfu.has(SubWindowRedrawingFlag::OVERHEAD));
    CHECK(rfu.has(SubWindowRedrawingFlag::DUNGEON));
}

TEST_CASE("RedrawingFlagsUpdater::request_sight_recalculation requests forgetting and recalculating the view and the lights")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    const auto restore = scoped_restore_flags(rfu, flags_srf);
    const auto restore_flow = scoped_restore_flags(rfu, { StatusRecalculatingFlag::FLOW });
    rfu.reset_flags(flags_srf);
    rfu.reset_flag(StatusRecalculatingFlag::FLOW);

    rfu.request_sight_recalculation();

    for (const auto flag : flags_srf) {
        CAPTURE(enum2i(flag));
        CHECK(rfu.has(flag));
    }

    // モンスターの経路は、地形が変わるときだけ呼び出し側で要求する
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::FLOW));
}

TEST_CASE("RedrawingFlagsUpdater::request_item_lists_redrawing requests redrawing the windows showing items")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
        SubWindowRedrawingFlag::PLAYER,
    };
    const auto restore = scoped_restore_flags(rfu, flags_swrf);
    rfu.reset_flags(flags_swrf);

    rfu.request_item_lists_redrawing();

    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK(rfu.has(flag));
    }
}

TEST_CASE("RedrawingFlagsUpdater::request_floor_item_lists_redrawing requests redrawing the windows showing floor items")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    const auto restore = scoped_restore_flags(rfu, flags_swrf);
    rfu.reset_flags(flags_swrf);

    rfu.request_floor_item_lists_redrawing();

    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK(rfu.has(flag));
    }
}

TEST_CASE("RedrawingFlagsUpdater::request_spell_info_redrawing requests redrawing the spell list and the item knowledge")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::SPELL,
        SubWindowRedrawingFlag::ITEM_KNOWLEDGE,
    };
    const auto restore = scoped_restore_flags(rfu, flags_swrf);
    rfu.reset_flags(flags_swrf);

    rfu.request_spell_info_redrawing();

    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK(rfu.has(flag));
    }
}

TEST_CASE("RedrawingFlagsUpdater::request_inscription_change_updates requests recalculating the bonuses and combining the inventory")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore_srf = scoped_restore_flags(rfu, { StatusRecalculatingFlag::BONUS });
    static constexpr auto flags_iaf = {
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    const auto restore_iaf = scoped_restore_flags(rfu, flags_iaf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
    };
    const auto restore_swrf = scoped_restore_flags(rfu, flags_swrf);
    rfu.reset_flag(StatusRecalculatingFlag::BONUS);
    rfu.reset_flags(flags_iaf);
    rfu.reset_flags(flags_swrf);

    rfu.request_inscription_change_updates();

    CHECK(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK(rfu.has(InventoryArrangementFlag::COMBINATION));
    CHECK_FALSE(rfu.has(InventoryArrangementFlag::REORDER));
    // 一覧の描き直しは能力の再計算が要求し、床の一覧は呼び出し側が要求するので、ここでは要求しない
    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK_FALSE(rfu.has(flag));
    }
}

TEST_CASE("RedrawingFlagsUpdater::request_equipment_change_updates requests recalculating the bonuses, the light radius and the max MP")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::HP,
    };
    const auto restore = scoped_restore_flags(rfu, flags_srf);
    const auto restore_mwrf = scoped_restore_flags(rfu, { MainWindowRedrawingFlag::EQUIPPY });
    const auto restore_swrf = scoped_restore_flags(rfu, { SubWindowRedrawingFlag::PLAYER });
    rfu.reset_flags(flags_srf);
    rfu.reset_flag(MainWindowRedrawingFlag::EQUIPPY);
    rfu.reset_flag(SubWindowRedrawingFlag::PLAYER);

    rfu.request_equipment_change_updates();

    CHECK(rfu.has(StatusRecalculatingFlag::BONUS));
    CHECK(rfu.has(StatusRecalculatingFlag::TORCH));
    CHECK(rfu.has(StatusRecalculatingFlag::MP));
    CHECK_FALSE(rfu.has(StatusRecalculatingFlag::HP));
    // 装備の記号やプレイヤー情報の描き直しは、装備する・外すコマンドの側で要求する
    CHECK_FALSE(rfu.has(MainWindowRedrawingFlag::EQUIPPY));
    CHECK_FALSE(rfu.has(SubWindowRedrawingFlag::PLAYER));
}
