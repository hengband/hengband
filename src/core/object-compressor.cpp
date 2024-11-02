#include "core/object-compressor.h"
#include "core/window-redrawer.h"
#include "floor/floor-list.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "system/baseitem-info.h"
#include "system/floor-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "view/display-messages.h"
#include <algorithm>

/*!
 * @brief グローバルオブジェクト配列の要素番号i1のオブジェクトを要素番号i2に移動する /
 * Move an object from index i1 to index i2 in the object list
 * @param i1 オブジェクト移動元の要素番号
 * @param i2 オブジェクト移動先の要素番号
 */
static void compact_objects_aux(FloorType *floor_ptr, OBJECT_IDX i1, OBJECT_IDX i2)
{
    if (i1 == i2) {
        return;
    }

    auto *o_ptr = &floor_ptr->o_list[i1];

    // モンスター所為アイテムリストもしくは床上アイテムリストの要素番号i1をi2に書き換える
    auto &list = get_o_idx_list_contains(floor_ptr, i1);
    std::replace(list.begin(), list.end(), i1, i2);

    // 要素番号i1のオブジェクトを要素番号i2に移動
    floor_ptr->o_list[i2] = floor_ptr->o_list[i1];
    o_ptr->wipe();
}

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
    ItemEntity *o_ptr;
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

    auto *floor_ptr = &FloorList::get_instance().get_floor(0);
    for (int num = 0, cnt = 1; num < size; cnt++) {
        int cur_lev = 5 * cnt;
        int cur_dis = 5 * (20 - cnt);
        for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++) {
            o_ptr = &floor_ptr->o_list[i];

            if (!o_ptr->is_valid() || (o_ptr->get_baseitem().level > cur_lev)) {
                continue;
            }

            POSITION y, x;
            if (o_ptr->is_held_by_monster()) {
                MonsterEntity *m_ptr;
                m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
                y = m_ptr->fy;
                x = m_ptr->fx;

                if (evaluate_percent(90)) {
                    continue;
                }
            } else {
                y = o_ptr->iy;
                x = o_ptr->ix;
            }

            if ((cur_dis > 0) && (distance(player_ptr->y, player_ptr->x, y, x) < cur_dis)) {
                continue;
            }

            int chance = 90;
            if (o_ptr->is_fixed_or_random_artifact() && (cnt < 1000)) {
                chance = 100;
            }

            if (evaluate_percent(chance)) {
                continue;
            }

            delete_object_idx(player_ptr, i);
            num++;
        }
    }

    for (OBJECT_IDX i = floor_ptr->o_max - 1; i >= 1; i--) {
        o_ptr = &floor_ptr->o_list[i];
        if (o_ptr->is_valid()) {
            continue;
        }

        compact_objects_aux(floor_ptr, floor_ptr->o_max - 1, i);
        floor_ptr->o_max--;
    }
}
