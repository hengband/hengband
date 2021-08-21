#include "spell-kind/spells-fetcher.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-update.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/object-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief アイテム引き寄せ処理 /
 * Fetch an item (teleport it right underneath the caster)
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param dir 魔法の発動方向
 * @param wgt 許容重量
 * @param require_los 射線の通りを要求するならばTRUE
 */
void fetch_item(player_type *caster_ptr, DIRECTION dir, WEIGHT wgt, bool require_los)
{
    grid_type *g_ptr;
    object_type *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];

    if (!caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].o_idx_list.empty()) {
        msg_print(_("自分の足の下にある物は取れません。", "You can't fetch when you're already standing on something."));
        return;
    }

    POSITION ty, tx;
    if (dir == 5 && target_okay(caster_ptr)) {
        tx = target_col;
        ty = target_row;

        if (distance(caster_ptr->y, caster_ptr->x, ty, tx) > get_max_range(caster_ptr)) {
            msg_print(_("そんなに遠くにある物は取れません！", "You can't fetch something that far away!"));
            return;
        }

        g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];
        if (g_ptr->o_idx_list.empty()) {
            msg_print(_("そこには何もありません。", "There is no object there."));
            return;
        }

        if (g_ptr->is_icky()) {
            msg_print(_("アイテムがコントロールを外れて落ちた。", "The item slips from your control."));
            return;
        }

        if (require_los) {
            if (!player_has_los_bold(caster_ptr, ty, tx)) {
                msg_print(_("そこはあなたの視界に入っていません。", "You have no direct line of sight to that location."));
                return;
            } else if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, ty, tx)) {
                msg_print(_("そこは壁の向こうです。", "You have no direct line of sight to that location."));
                return;
            }
        }
    } else {
        ty = caster_ptr->y;
        tx = caster_ptr->x;
        bool is_first_loop = true;
        g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];
        while (is_first_loop || g_ptr->o_idx_list.empty()) {
            is_first_loop = false;
            ty += ddy[dir];
            tx += ddx[dir];
            g_ptr = &caster_ptr->current_floor_ptr->grid_array[ty][tx];

            if ((distance(caster_ptr->y, caster_ptr->x, ty, tx) > get_max_range(caster_ptr))
                || !cave_has_flag_bold(caster_ptr->current_floor_ptr, ty, tx, FF::PROJECT))
                return;
        }
    }

    o_ptr = &caster_ptr->current_floor_ptr->o_list[g_ptr->o_idx_list.front()];
    if (o_ptr->weight > wgt) {
        msg_print(_("そのアイテムは重過ぎます。", "The object is too heavy."));
        return;
    }

    OBJECT_IDX i = g_ptr->o_idx_list.front();
    g_ptr->o_idx_list.pop_front();
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].o_idx_list.add(caster_ptr->current_floor_ptr, i); /* 'move' it */

    o_ptr->iy = caster_ptr->y;
    o_ptr->ix = caster_ptr->x;

    describe_flavor(caster_ptr, o_name, o_ptr, OD_NAME_ONLY);
    msg_format(_("%^sがあなたの足元に飛んできた。", "%^s flies through the air to your feet."), o_name);

    note_spot(caster_ptr, caster_ptr->y, caster_ptr->x);
    caster_ptr->redraw |= PR_MAP;
}

bool fetch_monster(player_type *caster_ptr)
{
    monster_type *m_ptr;
    MONSTER_IDX m_idx;
    GAME_TEXT m_name[MAX_NLEN];
    int i;
    int path_n;
    uint16_t path_g[512];
    POSITION ty, tx;

    if (!target_set(caster_ptr, TARGET_KILL))
        return false;

    m_idx = caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
    if (!m_idx)
        return false;
    if (m_idx == caster_ptr->riding)
        return false;
    if (!player_has_los_bold(caster_ptr, target_row, target_col))
        return false;
    if (!projectable(caster_ptr, caster_ptr->y, caster_ptr->x, target_row, target_col))
        return false;

    m_ptr = &caster_ptr->current_floor_ptr->m_list[m_idx];
    monster_desc(caster_ptr, m_name, m_ptr, 0);
    msg_format(_("%sを引き戻した。", "You pull back %s."), m_name);
    path_n = projection_path(caster_ptr, path_g, get_max_range(caster_ptr), target_row, target_col, caster_ptr->y, caster_ptr->x, 0);
    ty = target_row, tx = target_col;
    for (i = 1; i < path_n; i++) {
        POSITION ny = get_grid_y(path_g[i]);
        POSITION nx = get_grid_x(path_g[i]);
        grid_type *g_ptr = &caster_ptr->current_floor_ptr->grid_array[ny][nx];

        if (in_bounds(caster_ptr->current_floor_ptr, ny, nx) && is_cave_empty_bold(caster_ptr, ny, nx) && !g_ptr->is_object()
            && !pattern_tile(caster_ptr->current_floor_ptr, ny, nx)) {
            ty = ny;
            tx = nx;
        }
    }

    caster_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx = 0;
    caster_ptr->current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
    m_ptr->fy = ty;
    m_ptr->fx = tx;
    (void)set_monster_csleep(caster_ptr, m_idx, 0);
    update_monster(caster_ptr, m_idx, true);
    lite_spot(caster_ptr, target_row, target_col);
    lite_spot(caster_ptr, ty, tx);
    if (r_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK))
        caster_ptr->update |= (PU_MON_LITE);

    if (m_ptr->ml) {
        if (!caster_ptr->image)
            monster_race_track(caster_ptr, m_ptr->ap_r_idx);

        health_track(caster_ptr, m_idx);
    }

    return true;
}
