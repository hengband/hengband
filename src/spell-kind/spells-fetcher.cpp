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
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "timed-effect/player-hallucination.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief アイテム引き寄せ処理 /
 * Fetch an item (teleport it right underneath the caster)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 魔法の発動方向
 * @param wgt 許容重量
 * @param require_los 射線の通りを要求するならばTRUE
 */
void fetch_item(PlayerType *player_ptr, DIRECTION dir, WEIGHT wgt, bool require_los)
{
    grid_type *g_ptr;
    ItemEntity *o_ptr;
    GAME_TEXT o_name[MAX_NLEN];

    if (!player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].o_idx_list.empty()) {
        msg_print(_("自分の足の下にある物は取れません。", "You can't fetch when you're already standing on something."));
        return;
    }

    POSITION ty, tx;
    if (dir == 5 && target_okay(player_ptr)) {
        tx = target_col;
        ty = target_row;

        if (distance(player_ptr->y, player_ptr->x, ty, tx) > get_max_range(player_ptr)) {
            msg_print(_("そんなに遠くにある物は取れません！", "You can't fetch something that far away!"));
            return;
        }

        g_ptr = &player_ptr->current_floor_ptr->grid_array[ty][tx];
        if (g_ptr->o_idx_list.empty()) {
            msg_print(_("そこには何もありません。", "There is no object there."));
            return;
        }

        if (g_ptr->is_icky()) {
            msg_print(_("アイテムがコントロールを外れて落ちた。", "The item slips from your control."));
            return;
        }

        if (require_los) {
            if (!player_has_los_bold(player_ptr, ty, tx)) {
                msg_print(_("そこはあなたの視界に入っていません。", "You have no direct line of sight to that location."));
                return;
            } else if (!projectable(player_ptr, player_ptr->y, player_ptr->x, ty, tx)) {
                msg_print(_("そこは壁の向こうです。", "You have no direct line of sight to that location."));
                return;
            }
        }
    } else {
        ty = player_ptr->y;
        tx = player_ptr->x;
        bool is_first_loop = true;
        g_ptr = &player_ptr->current_floor_ptr->grid_array[ty][tx];
        while (is_first_loop || g_ptr->o_idx_list.empty()) {
            is_first_loop = false;
            ty += ddy[dir];
            tx += ddx[dir];
            g_ptr = &player_ptr->current_floor_ptr->grid_array[ty][tx];

            if ((distance(player_ptr->y, player_ptr->x, ty, tx) > get_max_range(player_ptr)) || !cave_has_flag_bold(player_ptr->current_floor_ptr, ty, tx, TerrainCharacteristics::PROJECT)) {
                return;
            }
        }
    }

    o_ptr = &player_ptr->current_floor_ptr->o_list[g_ptr->o_idx_list.front()];
    if (o_ptr->weight > wgt) {
        msg_print(_("そのアイテムは重過ぎます。", "The object is too heavy."));
        return;
    }

    OBJECT_IDX i = g_ptr->o_idx_list.front();
    g_ptr->o_idx_list.pop_front();
    player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].o_idx_list.add(player_ptr->current_floor_ptr, i); /* 'move' it */

    o_ptr->iy = player_ptr->y;
    o_ptr->ix = player_ptr->x;

    describe_flavor(player_ptr, o_name, o_ptr, OD_NAME_ONLY);
    msg_format(_("%^sがあなたの足元に飛んできた。", "%^s flies through the air to your feet."), o_name);

    note_spot(player_ptr, player_ptr->y, player_ptr->x);
    player_ptr->redraw |= PR_MAP;
}

bool fetch_monster(PlayerType *player_ptr)
{
    MonsterEntity *m_ptr;
    MONSTER_IDX m_idx;
    GAME_TEXT m_name[MAX_NLEN];
    POSITION ty, tx;

    if (!target_set(player_ptr, TARGET_KILL)) {
        return false;
    }

    m_idx = player_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx;
    if (!m_idx) {
        return false;
    }
    if (m_idx == player_ptr->riding) {
        return false;
    }
    if (!player_has_los_bold(player_ptr, target_row, target_col)) {
        return false;
    }
    if (!projectable(player_ptr, player_ptr->y, player_ptr->x, target_row, target_col)) {
        return false;
    }

    m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_desc(player_ptr, m_name, m_ptr, 0);
    msg_format(_("%sを引き戻した。", "You pull back %s."), m_name);
    projection_path path_g(player_ptr, get_max_range(player_ptr), target_row, target_col, player_ptr->y, player_ptr->x, 0);
    ty = target_row, tx = target_col;
    for (const auto &[ny, nx] : path_g) {
        auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[ny][nx];

        if (in_bounds(player_ptr->current_floor_ptr, ny, nx) && is_cave_empty_bold(player_ptr, ny, nx) && !g_ptr->is_object() && !pattern_tile(player_ptr->current_floor_ptr, ny, nx)) {
            ty = ny;
            tx = nx;
        }
    }

    player_ptr->current_floor_ptr->grid_array[target_row][target_col].m_idx = 0;
    player_ptr->current_floor_ptr->grid_array[ty][tx].m_idx = m_idx;
    m_ptr->fy = ty;
    m_ptr->fx = tx;
    (void)set_monster_csleep(player_ptr, m_idx, 0);
    update_monster(player_ptr, m_idx, true);
    lite_spot(player_ptr, target_row, target_col);
    lite_spot(player_ptr, ty, tx);
    if (monraces_info[m_ptr->r_idx].flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)) {
        player_ptr->update |= (PU_MON_LITE);
    }

    if (m_ptr->ml) {
        if (!player_ptr->effects()->hallucination()->is_hallucinated()) {
            monster_race_track(player_ptr, m_ptr->ap_r_idx);
        }

        health_track(player_ptr, m_idx);
    }

    return true;
}
