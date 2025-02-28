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
#include "view/display-messages.h"
#include <algorithm>
#include <range/v3/view.hpp>
#include <utility>

/*!
 * @brief グローバルオブジェクト配列から優先度の低いものを削除し、データを圧縮する。 /
 * Compact and Reorder the object list.
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
    for (int num = 0, cnt = 1; num < size; cnt++) {
        int cur_lev = 5 * cnt;
        int cur_dis = 5 * (20 - cnt);
        std::vector<OBJECT_IDX> delete_i_idx_list;
        for (const auto &[i_idx, item_ptr] : floor.o_list | ranges::views::enumerate) {
            if (!item_ptr->is_valid() || (item_ptr->get_baseitem_level() > cur_lev)) {
                continue;
            }

            POSITION y, x;
            if (item_ptr->is_held_by_monster()) {
                const auto &monster = floor.m_list[item_ptr->held_m_idx];
                y = monster.fy;
                x = monster.fx;

                if (evaluate_percent(90)) {
                    continue;
                }
            } else {
                y = item_ptr->iy;
                x = item_ptr->ix;
            }

            if ((cur_dis > 0) && (Grid::calc_distance(player_ptr->get_position(), { y, x }) < cur_dis)) {
                continue;
            }

            int chance = 90;
            if (item_ptr->is_fixed_or_random_artifact() && (cnt < 1000)) {
                chance = 100;
            }

            if (evaluate_percent(chance)) {
                continue;
            }

            delete_i_idx_list.push_back(static_cast<OBJECT_IDX>(i_idx));
            num++;
        }

        delete_items(player_ptr, std::move(delete_i_idx_list));
    }
}
