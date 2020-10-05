#include "core/object-compressor.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief グローバルオブジェクト配列に対し指定範囲のオブジェクトを整理してIDの若い順に寄せる /
 * Move an object from index i1 to index i2 in the object list
 * @param i1 整理したい配列の始点
 * @param i2 整理したい配列の終点
 * @return なし
 */
static void compact_objects_aux(floor_type *floor_ptr, OBJECT_IDX i1, OBJECT_IDX i2)
{
    if (i1 == i2)
        return;

    object_type *o_ptr;
    for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++) {
        o_ptr = &floor_ptr->o_list[i];
        if (!o_ptr->k_idx)
            continue;

        if (o_ptr->next_o_idx == i1)
            o_ptr->next_o_idx = i2;
    }

    o_ptr = &floor_ptr->o_list[i1];

    if (object_is_held_monster(o_ptr)) {
        monster_type *m_ptr;
        m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
        if (m_ptr->hold_o_idx == i1)
            m_ptr->hold_o_idx = i2;
    } else {
        POSITION y = o_ptr->iy;
        POSITION x = o_ptr->ix;
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        if (g_ptr->o_idx == i1)
            g_ptr->o_idx = i2;
    }

    floor_ptr->o_list[i2] = floor_ptr->o_list[i1];
    object_wipe(o_ptr);
}

/*!
 * @brief グローバルオブジェクト配列から優先度の低いものを削除し、データを圧縮する。 /
 * Compact and Reorder the object list.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param size 最低でも減らしたいオブジェクト数の水準
 * @return なし
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
void compact_objects(player_type *player_ptr, int size)
{
    object_type *o_ptr;
    if (size) {
        msg_print(_("アイテム情報を圧縮しています...", "Compacting objects..."));
        player_ptr->redraw |= PR_MAP;
        player_ptr->window |= PW_OVERHEAD | PW_DUNGEON;
    }

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int num = 0, cnt = 1; num < size; cnt++) {
        int cur_lev = 5 * cnt;
        int cur_dis = 5 * (20 - cnt);
        for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++) {
            o_ptr = &floor_ptr->o_list[i];

            if (!object_is_valid(o_ptr) || (k_info[o_ptr->k_idx].level > cur_lev))
                continue;

            POSITION y, x;
            if (object_is_held_monster(o_ptr)) {
                monster_type *m_ptr;
                m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
                y = m_ptr->fy;
                x = m_ptr->fx;

                if (randint0(100) < 90)
                    continue;
            } else {
                y = o_ptr->iy;
                x = o_ptr->ix;
            }

            if ((cur_dis > 0) && (distance(player_ptr->y, player_ptr->x, y, x) < cur_dis))
                continue;

            int chance = 90;
            if ((object_is_fixed_artifact(o_ptr) || o_ptr->art_name) && (cnt < 1000))
                chance = 100;

            if (randint0(100) < chance)
                continue;

            delete_object_idx(player_ptr, i);
            num++;
        }
    }

    for (OBJECT_IDX i = floor_ptr->o_max - 1; i >= 1; i--) {
        o_ptr = &floor_ptr->o_list[i];
        if (o_ptr->k_idx)
            continue;

        compact_objects_aux(floor_ptr, floor_ptr->o_max - 1, i);
        floor_ptr->o_max--;
    }
}
