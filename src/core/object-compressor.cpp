#include "core/object-compressor.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/z-rand.h"
#include "view/display-messages.h"
#include <range/v3/view.hpp>
#include <utility>

namespace {
class ItemCompactionChecker {
public:
    ItemCompactionChecker(PlayerType *player_ptr, int try_count)
        : player_ptr(player_ptr)
        , try_count(try_count)
        , level_threshold(5 * try_count)
        , distance_threshold(5 * (20 - try_count))
    {
    }

    bool can_delete_for_compaction(const ItemEntity &item) const
    {
        if (!item.is_valid() || (item.get_baseitem_level() > this->level_threshold)) {
            return false;
        }

        if (item.is_held_by_monster() && evaluate_percent(90)) {
            return false;
        }

        const auto &floor = *player_ptr->current_floor_ptr;
        const auto pos = item.is_held_by_monster() ? floor.m_list[item.held_m_idx].get_position() : item.get_position();

        if (Grid::calc_distance(player_ptr->get_position(), pos) < this->distance_threshold) {
            return false;
        }

        if (item.is_fixed_or_random_artifact() && (try_count < 1000)) {
            return false;
        }

        return evaluate_percent(10);
    }

private:
    PlayerType *player_ptr;
    int try_count;
    int level_threshold;
    int distance_threshold;
};
}

/*!
 * @brief アイテム配列から優先度の低いものを削除する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param size 最低でも減らしたいオブジェクト数の水準
 * @details
 * （危険なので使用には注意すること）
 * This function can be very dangerous, use with caution!\n
 *\n
 * When actually "compacting" objects, we base the saving throw on a\n
 * combination of object level, distance from player, and current\n
 * "desperation".\n
 *\n
 * After "compacting" (if needed), we "reorder" the objects into a more\n
 * compact order, and we reset the allocation info, and the "live" array.\n
 */
void compact_objects(PlayerType *player_ptr, int size)
{
    if (size) {
        msg_print(_("アイテム情報を圧縮しています...", "Compacting objects..."));
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        rfu.set_flag(MainWindowRedrawingFlag::MAP);
        static constexpr auto flags_swrf = {
            SubWindowRedrawingFlag::OVERHEAD,
            SubWindowRedrawingFlag::DUNGEON,
        };
        rfu.set_flags(flags_swrf);
    }

    auto &floor = *player_ptr->current_floor_ptr;
    for (auto deleted_num = 0, try_count = 1; deleted_num < size; try_count++) {
        const ItemCompactionChecker icc(player_ptr, try_count);
        std::vector<OBJECT_IDX> delete_i_idx_list;
        for (const auto &[i_idx, item_ptr] : floor.o_list | ranges::views::enumerate) {
            if (icc.can_delete_for_compaction(*item_ptr)) {
                delete_i_idx_list.push_back(static_cast<OBJECT_IDX>(i_idx));
            }
        }

        deleted_num += delete_i_idx_list.size();
        delete_items(player_ptr, std::move(delete_i_idx_list));
    }
}
