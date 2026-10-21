/*!
 * @brief DeferredUpdateRequests のテスト
 */

#include "system/deferred-update-requests.h"
#include "util/enum-converter.h"
#include "util/finalizer.h"
#include "util/flag-group.h"
#include <doctest/doctest.h>
#include <initializer_list>
#include <vector>

namespace {

/*!
 * @brief フラグの今の状態を覚えておき、スコープを抜けるときに元の状態に戻す
 * @tparam Flag フラグの列挙型。1 回の呼び出しで戻せるのは同じ列挙型のフラグだけ
 * @param dur 更新フラグ
 * @param flags 戻すフラグ
 * @return スコープを抜けるときにフラグを戻すファイナライザ
 */
template <RedrawingFlagType Flag>
auto scoped_restore_flags(DeferredUpdateRequests &dur, std::initializer_list<Flag> flags)
{
    const EnumClassFlagGroup<Flag> targets(flags);
    EnumClassFlagGroup<Flag> was_set;
    for (const auto flag : flags) {
        if (dur.has(flag)) {
            was_set.set(flag);
        }
    }

    return util::make_finalizer([&dur, targets, was_set] {
        dur.reset_flags(targets);
        dur.set_flags(was_set);
    });
}

}

static_assert(RedrawingFlagType<MainWindowRedrawingFlag>);
static_assert(RedrawingFlagType<SubWindowRedrawingFlag>);
static_assert(RedrawingFlagType<StatusRecalculatingFlag>);
static_assert(RedrawingFlagType<InventoryArrangementFlag>);
static_assert(!RedrawingFlagType<int>);

TEST_CASE("DeferredUpdateRequests operates only on the flag group of the given flag type")
{
    // 4 種類のフラグで、同じ値 (0) の列挙子を使う
    const auto for_each_flag = [](auto func) {
        func(MainWindowRedrawingFlag::TITLE);
        func(SubWindowRedrawingFlag::INVENTORY);
        func(StatusRecalculatingFlag::BONUS);
        func(InventoryArrangementFlag::AUTO_DESTRUCTION);
    };
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore_mwrf = scoped_restore_flags(dur, { MainWindowRedrawingFlag::TITLE });
    const auto restore_swrf = scoped_restore_flags(dur, { SubWindowRedrawingFlag::INVENTORY });
    const auto restore_srf = scoped_restore_flags(dur, { StatusRecalculatingFlag::BONUS });
    const auto restore_iaf = scoped_restore_flags(dur, { InventoryArrangementFlag::AUTO_DESTRUCTION });
    const auto states = [&] {
        std::vector<bool> result;
        for_each_flag([&](auto flag) { result.push_back(dur.has(flag)); });
        return result;
    };

    // set で指定したフラグだけが立ち、ほかの型のフラグを全て立てたまま reset で指定したフラグだけが消えることを確かめる
    const auto check_each_type = [&](auto set, auto reset) {
        size_t index = 0;
        for_each_flag([&](auto flag) {
            CAPTURE(index);
            for_each_flag([&](auto f) { dur.reset_flag(f); });
            set(flag);
            std::vector<bool> expected(4, false);
            expected[index] = true;
            CHECK(states() == expected);
            CHECK(dur.has_any_of({ flag }));
            CHECK(dur.has_any_of(EnumClassFlagGroup<decltype(flag)>{ flag }));

            for_each_flag([&](auto f) { dur.set_flag(f); });
            reset(flag);
            expected.assign(4, true);
            expected[index] = false;
            CHECK(states() == expected);
            CHECK_FALSE(dur.has_any_of({ flag }));
            CHECK_FALSE(dur.has_any_of(EnumClassFlagGroup<decltype(flag)>{ flag }));
            ++index;
        });
    };

    SUBCASE("set_flag and reset_flag")
    {
        check_each_type([&dur](auto flag) { dur.set_flag(flag); }, [&dur](auto flag) { dur.reset_flag(flag); });
    }

    SUBCASE("set_flags and reset_flags with an initializer list")
    {
        check_each_type([&dur](auto flag) { dur.set_flags({ flag }); }, [&dur](auto flag) { dur.reset_flags({ flag }); });
    }

    SUBCASE("set_flags and reset_flags with a flag group")
    {
        check_each_type([&dur](auto flag) { dur.set_flags(EnumClassFlagGroup<decltype(flag)>{ flag }); }, [&dur](auto flag) { dur.reset_flags(EnumClassFlagGroup<decltype(flag)>{ flag }); });
    }
}

TEST_CASE("DeferredUpdateRequests::request_inventory_arrangement requests combining, reordering and redrawing the inventory")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_iaf = {
        InventoryArrangementFlag::AUTO_DESTRUCTION,
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    const auto restore_iaf = scoped_restore_flags(dur, flags_iaf);
    const auto restore_swrf = scoped_restore_flags(dur, { SubWindowRedrawingFlag::INVENTORY });
    dur.reset_flags(flags_iaf);
    dur.reset_flag(SubWindowRedrawingFlag::INVENTORY);

    dur.request_inventory_arrangement();

    CHECK(dur.has(InventoryArrangementFlag::COMBINATION));
    CHECK(dur.has(InventoryArrangementFlag::REORDER));
    CHECK(dur.has(SubWindowRedrawingFlag::INVENTORY));
    CHECK_FALSE(dur.has(InventoryArrangementFlag::AUTO_DESTRUCTION));
}

TEST_CASE("DeferredUpdateRequests::request_whole_main_window_redrawing requests wiping and redrawing the whole main window")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    const auto restore = scoped_restore_flags(dur, flags_mwrf);
    dur.reset_flags(flags_mwrf);

    dur.request_whole_main_window_redrawing();

    for (const auto flag : flags_mwrf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_whole_status_recalculation requests recalculating the status, the view and the lights for redrawing the whole screen")
{
    auto &dur = DeferredUpdateRequests::get_instance();
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
    const auto restore = scoped_restore_flags(dur, flags_srf);
    const auto restore_flow = scoped_restore_flags(dur, { StatusRecalculatingFlag::FLOW, StatusRecalculatingFlag::DISTANCE });
    dur.reset_flags(flags_srf);
    dur.reset_flag(StatusRecalculatingFlag::FLOW);
    dur.reset_flag(StatusRecalculatingFlag::DISTANCE);

    dur.request_whole_status_recalculation();

    for (const auto flag : flags_srf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }

    // モンスターの経路と距離は、フロアに入ったときなどに別に計算する
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::FLOW));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::DISTANCE));
}

TEST_CASE("DeferredUpdateRequests::request_map_redrawing requests redrawing the map of the main window")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    const auto restore_mwrf = scoped_restore_flags(dur, { MainWindowRedrawingFlag::MAP });
    const auto restore_swrf = scoped_restore_flags(dur, flags_swrf);
    dur.reset_flag(MainWindowRedrawingFlag::MAP);
    dur.reset_flags(flags_swrf);

    dur.request_map_redrawing();

    CHECK(dur.has(MainWindowRedrawingFlag::MAP));
    // 地図のサブウィンドウは、マップを描き直すとき (print_map()) に要求される
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::OVERHEAD));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::DUNGEON));
}

TEST_CASE("DeferredUpdateRequests::request_monster_update_and_map_redrawing requests updating the monsters and redrawing the map of the main window")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    const auto restore_srf = scoped_restore_flags(dur, { StatusRecalculatingFlag::MONSTER_STATUSES });
    const auto restore_mwrf = scoped_restore_flags(dur, { MainWindowRedrawingFlag::MAP });
    const auto restore_swrf = scoped_restore_flags(dur, flags_swrf);
    dur.reset_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    dur.reset_flag(MainWindowRedrawingFlag::MAP);
    dur.reset_flags(flags_swrf);

    dur.request_monster_update_and_map_redrawing();

    CHECK(dur.has(StatusRecalculatingFlag::MONSTER_STATUSES));
    CHECK(dur.has(MainWindowRedrawingFlag::MAP));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::OVERHEAD));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::DUNGEON));
}

TEST_CASE("DeferredUpdateRequests::request_sight_recalculation requests forgetting and recalculating the view and the lights")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
    };
    const auto restore = scoped_restore_flags(dur, flags_srf);
    const auto restore_flow = scoped_restore_flags(dur, { StatusRecalculatingFlag::FLOW });
    dur.reset_flags(flags_srf);
    dur.reset_flag(StatusRecalculatingFlag::FLOW);

    dur.request_sight_recalculation();

    for (const auto flag : flags_srf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }

    // モンスターの経路は、地形が変わるときだけ呼び出し側で要求する
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::FLOW));
}

TEST_CASE("DeferredUpdateRequests::request_item_lists_redrawing requests redrawing the windows showing items")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
        SubWindowRedrawingFlag::PLAYER,
    };
    const auto restore = scoped_restore_flags(dur, flags_swrf);
    dur.reset_flags(flags_swrf);

    dur.request_item_lists_redrawing();

    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_carried_item_lists_redrawing requests redrawing the inventory and the equipment")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
    };
    const auto restore = scoped_restore_flags(dur, flags_swrf);
    static constexpr auto flags_swrf_others = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
        SubWindowRedrawingFlag::PLAYER,
    };
    const auto restore_others = scoped_restore_flags(dur, flags_swrf_others);
    dur.reset_flags(flags_swrf);
    dur.reset_flags(flags_swrf_others);

    dur.request_carried_item_lists_redrawing();

    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }

    // 床のアイテムの一覧とプレイヤーの情報は request_item_lists_redrawing() が要求する
    for (const auto flag : flags_swrf_others) {
        CAPTURE(enum2i(flag));
        CHECK_FALSE(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_floor_item_lists_redrawing requests redrawing the windows showing floor items")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    const auto restore = scoped_restore_flags(dur, flags_swrf);
    dur.reset_flags(flags_swrf);

    dur.request_floor_item_lists_redrawing();

    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_spell_info_redrawing requests redrawing the spell list and the item knowledge")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::SPELL,
        SubWindowRedrawingFlag::ITEM_KNOWLEDGE,
    };
    const auto restore = scoped_restore_flags(dur, flags_swrf);
    dur.reset_flags(flags_swrf);

    dur.request_spell_info_redrawing();

    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_inscription_change_updates requests recalculating the bonuses and combining the inventory")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore_srf = scoped_restore_flags(dur, { StatusRecalculatingFlag::BONUS });
    static constexpr auto flags_iaf = {
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    const auto restore_iaf = scoped_restore_flags(dur, flags_iaf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
    };
    const auto restore_swrf = scoped_restore_flags(dur, flags_swrf);
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flags(flags_iaf);
    dur.reset_flags(flags_swrf);

    dur.request_inscription_change_updates();

    CHECK(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK(dur.has(InventoryArrangementFlag::COMBINATION));
    CHECK_FALSE(dur.has(InventoryArrangementFlag::REORDER));
    // 一覧の描き直しは能力の再計算が要求し、床の一覧は呼び出し側が要求するので、ここでは要求しない
    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK_FALSE(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_equipment_change_updates requests recalculating the bonuses, the light radius and the max MP")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::HP,
    };
    const auto restore = scoped_restore_flags(dur, flags_srf);
    const auto restore_mwrf = scoped_restore_flags(dur, { MainWindowRedrawingFlag::EQUIPPY });
    const auto restore_swrf = scoped_restore_flags(dur, { SubWindowRedrawingFlag::PLAYER });
    dur.reset_flags(flags_srf);
    dur.reset_flag(MainWindowRedrawingFlag::EQUIPPY);
    dur.reset_flag(SubWindowRedrawingFlag::PLAYER);

    dur.request_equipment_change_updates();

    CHECK(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK(dur.has(StatusRecalculatingFlag::TORCH));
    CHECK(dur.has(StatusRecalculatingFlag::MP));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::HP));
    // 装備の記号やプレイヤー情報の描き直しは、装備する・外すコマンドの側で要求する
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::EQUIPPY));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::PLAYER));
}

TEST_CASE("DeferredUpdateRequests::request_riding_status_redrawing requests redrawing the riding status and the mount's health")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::UHEALTH,
        MainWindowRedrawingFlag::HEALTH,
        MainWindowRedrawingFlag::MAP,
    };
    const auto restore_mwrf = scoped_restore_flags(dur, flags_mwrf);
    const auto restore_srf = scoped_restore_flags(dur, { StatusRecalculatingFlag::BONUS });
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    const auto restore_swrf = scoped_restore_flags(dur, flags_swrf);
    dur.reset_flags(flags_mwrf);
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flags(flags_swrf);

    dur.request_riding_status_redrawing();

    CHECK(dur.has(MainWindowRedrawingFlag::EXTRA));
    CHECK(dur.has(MainWindowRedrawingFlag::UHEALTH));
    // 追跡しているモンスターの体力や、乗っているモンスターで変わる能力の再計算は、それぞれの箇所で要求する
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::HEALTH));
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::BONUS));
    // 乗り降りのコマンドで一緒に要求するメイン画面の地図 (MAP) と、落馬で一緒に要求するサブウィンドウの地図
    // (OVERHEAD・DUNGEON) の描き直しも、それぞれの箇所で要求する
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::MAP));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::OVERHEAD));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::DUNGEON));
}

TEST_CASE("DeferredUpdateRequests::request_hp_mp_redrawing requests redrawing the HP and the MP")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    };
    static constexpr auto flags_mwrf_others = {
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
    };
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::SPELL,
        SubWindowRedrawingFlag::ITEM_KNOWLEDGE,
        SubWindowRedrawingFlag::PLAYER,
    };
    const auto restore_mwrf = scoped_restore_flags(dur, flags_mwrf);
    const auto restore_mwrf_others = scoped_restore_flags(dur, flags_mwrf_others);
    const auto restore_swrf = scoped_restore_flags(dur, flags_swrf);
    dur.reset_flags(flags_mwrf);
    dur.reset_flags(flags_mwrf_others);
    dur.reset_flags(flags_swrf);

    dur.request_hp_mp_redrawing();

    for (const auto flag : flags_mwrf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }

    // 基本ステータスの全体や拡張ステータスは描き直さない
    for (const auto flag : flags_mwrf_others) {
        CAPTURE(enum2i(flag));
        CHECK_FALSE(dur.has(flag));
    }

    // 呪文の情報は HP・MP を描き直すときに、プレイヤーの情報は前に描いた HP・MP と比べて要求される
    for (const auto flag : flags_swrf) {
        CAPTURE(enum2i(flag));
        CHECK_FALSE(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_gold_redrawing requests redrawing the gold and the player window")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore_mwrf = scoped_restore_flags(dur, { MainWindowRedrawingFlag::GOLD, MainWindowRedrawingFlag::BASIC });
    const auto restore_swrf = scoped_restore_flags(dur, { SubWindowRedrawingFlag::PLAYER });
    dur.reset_flag(MainWindowRedrawingFlag::GOLD);
    dur.reset_flag(MainWindowRedrawingFlag::BASIC);
    dur.reset_flag(SubWindowRedrawingFlag::PLAYER);

    dur.request_gold_redrawing();

    CHECK(dur.has(MainWindowRedrawingFlag::GOLD));
    CHECK(dur.has(SubWindowRedrawingFlag::PLAYER));
    // 所持金のほかの基本ステータスは描き直さない
    CHECK_FALSE(dur.has(MainWindowRedrawingFlag::BASIC));
}

TEST_CASE("DeferredUpdateRequests::request_light_fuel_change_updates requests recalculating the light and redrawing the equipment")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    const auto restore_srf = scoped_restore_flags(dur, { StatusRecalculatingFlag::TORCH, StatusRecalculatingFlag::BONUS });
    const auto restore_swrf = scoped_restore_flags(dur, { SubWindowRedrawingFlag::EQUIPMENT, SubWindowRedrawingFlag::INVENTORY });
    dur.reset_flag(StatusRecalculatingFlag::TORCH);
    dur.reset_flag(StatusRecalculatingFlag::BONUS);
    dur.reset_flag(SubWindowRedrawingFlag::EQUIPMENT);
    dur.reset_flag(SubWindowRedrawingFlag::INVENTORY);

    dur.request_light_fuel_change_updates();

    CHECK(dur.has(StatusRecalculatingFlag::TORCH));
    CHECK(dur.has(SubWindowRedrawingFlag::EQUIPMENT));
    // 能力 (燃料には依らない) の再計算や、所持品の一覧の描き直しは要求しない
    CHECK_FALSE(dur.has(StatusRecalculatingFlag::BONUS));
    CHECK_FALSE(dur.has(SubWindowRedrawingFlag::INVENTORY));
}

TEST_CASE("DeferredUpdateRequests::request_bonus_and_max_hp_recalculation requests recalculating the bonuses and the max HP")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
    };
    static constexpr auto flags_srf_others = {
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    const auto restore_srf = scoped_restore_flags(dur, flags_srf);
    const auto restore_srf_others = scoped_restore_flags(dur, flags_srf_others);
    dur.reset_flags(flags_srf);
    dur.reset_flags(flags_srf_others);

    dur.request_bonus_and_max_hp_recalculation();

    for (const auto flag : flags_srf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }

    // 最大 MP・覚えられる呪文の数・光源の明るさ・モンスターのステータスは計算し直さない
    for (const auto flag : flags_srf_others) {
        CAPTURE(enum2i(flag));
        CHECK_FALSE(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_max_mp_and_spells_recalculation requests recalculating the max MP and the spells")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    static constexpr auto flags_srf_others = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    const auto restore_srf = scoped_restore_flags(dur, flags_srf);
    const auto restore_srf_others = scoped_restore_flags(dur, flags_srf_others);
    dur.reset_flags(flags_srf);
    dur.reset_flags(flags_srf_others);

    dur.request_max_mp_and_spells_recalculation();

    for (const auto flag : flags_srf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }

    // 能力・最大 HP・光源の明るさ・モンスターのステータスは計算し直さない
    for (const auto flag : flags_srf_others) {
        CAPTURE(enum2i(flag));
        CHECK_FALSE(dur.has(flag));
    }
}

TEST_CASE("DeferredUpdateRequests::request_player_status_recalculation requests recalculating the bonuses, the max HP, the max MP and the spells")
{
    auto &dur = DeferredUpdateRequests::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    };
    static constexpr auto flags_srf_others = {
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    const auto restore_srf = scoped_restore_flags(dur, flags_srf);
    const auto restore_srf_others = scoped_restore_flags(dur, flags_srf_others);
    dur.reset_flags(flags_srf);
    dur.reset_flags(flags_srf_others);

    dur.request_player_status_recalculation();

    for (const auto flag : flags_srf) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag));
    }

    // 光源の明るさとモンスターのステータスは、必要な箇所で別に要求する
    for (const auto flag : flags_srf_others) {
        CAPTURE(enum2i(flag));
        CHECK_FALSE(dur.has(flag));
    }
}
