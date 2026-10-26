/*!
 * @brief SpellHex のテスト
 */

#include "player-base/player-class.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "system/deferred-update-requests.h"
#include "system/player-type-definition.h"
#include "test/scoped-redrawing-flags.h"
#include "util/enum-converter.h"
#include "util/enum-range.h"
#include "util/flag-group.h"
#include <doctest/doctest.h>

namespace {
/*!
 * @brief 指定した型の更新フラグをすべて消す
 * @tparam Flag フラグの列挙型
 */
template <RedrawingFlagType Flag>
void clear_all_flags()
{
    DeferredUpdateRequests::get_instance().reset_flags(EnumClassFlagGroup<Flag>(EnumRange(static_cast<Flag>(0), Flag::MAX)));
}

/*!
 * @brief 指定した型の更新フラグのうち、期待するものだけが立っていることを確かめる
 * @tparam Flag フラグの列挙型
 * @param expected 立っているはずのフラグ
 */
template <RedrawingFlagType Flag>
void check_only_flags_set(const EnumClassFlagGroup<Flag> &expected)
{
    const auto &dur = DeferredUpdateRequests::get_instance();
    for (const auto flag : EnumRange(static_cast<Flag>(0), Flag::MAX)) {
        CAPTURE(enum2i(flag));
        CHECK(dur.has(flag) == expected.has(flag));
    }
}
}

TEST_CASE("SpellHex::request_casting_change_updates requests recalculating the player status and redrawing the extra status, the HP and the MP")
{
    const auto restore_mwrf = test::scoped_restore_redrawing_flags<MainWindowRedrawingFlag>();
    const auto restore_swrf = test::scoped_restore_redrawing_flags<SubWindowRedrawingFlag>();
    const auto restore_srf = test::scoped_restore_redrawing_flags<StatusRecalculatingFlag>();
    const auto restore_iaf = test::scoped_restore_redrawing_flags<InventoryArrangementFlag>();
    clear_all_flags<MainWindowRedrawingFlag>();
    clear_all_flags<SubWindowRedrawingFlag>();
    clear_all_flags<StatusRecalculatingFlag>();
    clear_all_flags<InventoryArrangementFlag>();

    SpellHex::request_casting_change_updates();

    check_only_flags_set(EnumClassFlagGroup<StatusRecalculatingFlag>({
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
    }));
    check_only_flags_set(EnumClassFlagGroup<MainWindowRedrawingFlag>({
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::HP,
        MainWindowRedrawingFlag::MP,
    }));
    check_only_flags_set(EnumClassFlagGroup<SubWindowRedrawingFlag>());
    check_only_flags_set(EnumClassFlagGroup<InventoryArrangementFlag>());
}

TEST_CASE("SpellHex revenge setters request redrawing the status bar only when the shown revenge changes")
{
    const auto restore_mwrf = test::scoped_restore_redrawing_flags<MainWindowRedrawingFlag>();
    auto &dur = DeferredUpdateRequests::get_instance();
    PlayerType player;
    player.pclass = PlayerClassType::HIGH_MAGE;
    player.realm1 = RealmType::HEX;
    PlayerClass(&player).init_specific_data();
    SpellHex spell_hex(&player);

    SUBCASE("setting the type without remaining turns does not change the display")
    {
        clear_all_flags<MainWindowRedrawingFlag>();
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        CHECK_FALSE(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("starting, counting down and ending the revenge")
    {
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        clear_all_flags<MainWindowRedrawingFlag>();
        spell_hex.set_revenge_turn(2, true);
        CHECK(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        clear_all_flags<MainWindowRedrawingFlag>();
        spell_hex.set_revenge_turn(1, false);
        CHECK_FALSE(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        spell_hex.set_revenge_turn(1, false);
        CHECK(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("changing the type while it is shown")
    {
        spell_hex.set_revenge_type(SpellHexRevengeType::PATIENCE);
        spell_hex.set_revenge_turn(2, true);
        clear_all_flags<MainWindowRedrawingFlag>();
        spell_hex.set_revenge_type(SpellHexRevengeType::REVENGE);
        CHECK(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));
    }

    SUBCASE("clearing the revenge")
    {
        clear_all_flags<MainWindowRedrawingFlag>();
        spell_hex.clear_revenge();
        CHECK_FALSE(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));

        spell_hex.set_revenge_type(SpellHexRevengeType::REVENGE);
        spell_hex.set_revenge_turn(2, true);
        clear_all_flags<MainWindowRedrawingFlag>();
        spell_hex.clear_revenge();
        CHECK(dur.has(MainWindowRedrawingFlag::TIMED_EFFECT));
        CHECK(spell_hex.get_revenge_turn() == 0);
        CHECK(spell_hex.get_revenge_type() == SpellHexRevengeType::NONE);
    }
}
