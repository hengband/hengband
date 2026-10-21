/*!
 * @brief SpellHex のテスト
 */

#include "spell-realm/spells-hex.h"
#include "system/deferred-update-requests.h"
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
