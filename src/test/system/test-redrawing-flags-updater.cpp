/*!
 * @brief RedrawingFlagsUpdater のテスト
 */

#include "system/redrawing-flags-updater.h"
#include "util/finalizer.h"
#include <doctest/doctest.h>

namespace {

/*!
 * @brief フラグを元の状態に戻す
 * @param rfu 更新フラグ
 * @param flag 戻すフラグ
 * @param was_set 元の状態でフラグが立っていたか
 */
template <typename Flag>
void restore_flag(RedrawingFlagsUpdater &rfu, Flag flag, bool was_set)
{
    if (was_set) {
        rfu.set_flag(flag);
    } else {
        rfu.reset_flag(flag);
    }
}

}

TEST_CASE("RedrawingFlagsUpdater::request_inventory_arrangement requests combining, reordering and redrawing the inventory")
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    const auto restore = util::make_finalizer([&rfu,
                                                  auto_destruction = rfu.has(InventoryArrangementFlag::AUTO_DESTRUCTION),
                                                  combination = rfu.has(InventoryArrangementFlag::COMBINATION),
                                                  reorder = rfu.has(InventoryArrangementFlag::REORDER),
                                                  inventory = rfu.has(SubWindowRedrawingFlag::INVENTORY)] {
        restore_flag(rfu, InventoryArrangementFlag::AUTO_DESTRUCTION, auto_destruction);
        restore_flag(rfu, InventoryArrangementFlag::COMBINATION, combination);
        restore_flag(rfu, InventoryArrangementFlag::REORDER, reorder);
        restore_flag(rfu, SubWindowRedrawingFlag::INVENTORY, inventory);
    });
    static constexpr auto flags_iaf = {
        InventoryArrangementFlag::AUTO_DESTRUCTION,
        InventoryArrangementFlag::COMBINATION,
        InventoryArrangementFlag::REORDER,
    };
    rfu.reset_flags(flags_iaf);
    rfu.reset_flag(SubWindowRedrawingFlag::INVENTORY);

    rfu.request_inventory_arrangement();

    CHECK(rfu.has(InventoryArrangementFlag::COMBINATION));
    CHECK(rfu.has(InventoryArrangementFlag::REORDER));
    CHECK(rfu.has(SubWindowRedrawingFlag::INVENTORY));
    CHECK_FALSE(rfu.has(InventoryArrangementFlag::AUTO_DESTRUCTION));
}
