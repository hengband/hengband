#include "spell-kind/spells-fetcher.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-brightness-mask.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-update.h"
#include "system/angband-system.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
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
    auto &floor = *player_ptr->current_floor_ptr;
    const Pos2D p_pos(player_ptr->y, player_ptr->x);
    if (!floor.get_grid(p_pos).o_idx_list.empty()) {
        msg_print(_("自分の足の下にある物は取れません。", "You can't fetch when you're already standing on something."));
        return;
    }

    POSITION ty, tx;
    grid_type *g_ptr;
    const auto &system = AngbandSystem::get_instance();
    if (dir == 5 && target_okay(player_ptr)) {
        tx = target_col;
        ty = target_row;
        const Pos2D pos(ty, tx);
        if (distance(p_pos.y, p_pos.x, ty, tx) > system.get_max_range()) {
            msg_print(_("そんなに遠くにある物は取れません！", "You can't fetch something that far away!"));
            return;
        }

        g_ptr = &floor.get_grid(pos);
        if (g_ptr->o_idx_list.empty()) {
            msg_print(_("そこには何もありません。", "There is no object there."));
            return;
        }

        if (g_ptr->is_icky()) {
            msg_print(_("アイテムがコントロールを外れて落ちた。", "The item slips from your control."));
            return;
        }

        if (require_los) {
            if (!floor.has_los(pos)) {
                msg_print(_("そこはあなたの視界に入っていません。", "You have no direct line of sight to that location."));
                return;
            } else if (!projectable(player_ptr, p_pos.y, p_pos.x, ty, tx)) {
                msg_print(_("そこは壁の向こうです。", "You have no direct line of sight to that location."));
                return;
            }
        }
    } else {
        ty = p_pos.y;
        tx = p_pos.x;
        bool is_first_loop = true;
        g_ptr = &floor.grid_array[ty][tx];
        while (is_first_loop || g_ptr->o_idx_list.empty()) {
            is_first_loop = false;
            ty += ddy[dir];
            tx += ddx[dir];
            g_ptr = &floor.grid_array[ty][tx];
            if ((distance(p_pos.y, p_pos.x, ty, tx) > system.get_max_range())) {
                return;
            }

            if (!cave_has_flag_bold(&floor, ty, tx, TerrainCharacteristics::PROJECT)) {
                return;
            }
        }
    }

    auto *o_ptr = &floor.o_list[g_ptr->o_idx_list.front()];
    if (o_ptr->weight > wgt) {
        msg_print(_("そのアイテムは重過ぎます。", "The object is too heavy."));
        return;
    }

    OBJECT_IDX i = g_ptr->o_idx_list.front();
    g_ptr->o_idx_list.pop_front();
    floor.grid_array[p_pos.y][p_pos.x].o_idx_list.add(&floor, i); /* 'move' it */
    o_ptr->iy = p_pos.y;
    o_ptr->ix = p_pos.x;

    const auto item_name = describe_flavor(player_ptr, o_ptr, OD_NAME_ONLY);
    msg_format(_("%s^があなたの足元に飛んできた。", "%s^ flies through the air to your feet."), item_name.data());
    note_spot(player_ptr, p_pos.y, p_pos.x);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MAP);
}

bool fetch_monster(PlayerType *player_ptr)
{
    if (!target_set(player_ptr, TARGET_KILL)) {
        return false;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    const Pos2D pos(target_row, target_col);
    auto m_idx = floor.get_grid(pos).m_idx;
    if (!m_idx) {
        return false;
    }
    if (m_idx == player_ptr->riding) {
        return false;
    }
    if (!floor.has_los(pos)) {
        return false;
    }
    if (!projectable(player_ptr, player_ptr->y, player_ptr->x, target_row, target_col)) {
        return false;
    }

    auto &monster = floor.m_list[m_idx];
    const auto m_name = monster_desc(player_ptr, &monster, 0);
    msg_format(_("%sを引き戻した。", "You pull back %s."), m_name.data());
    projection_path path_g(player_ptr, AngbandSystem::get_instance().get_max_range(), target_row, target_col, player_ptr->y, player_ptr->x, 0);
    auto ty = target_row, tx = target_col;
    for (const auto &[ny, nx] : path_g) {
        const Pos2D pos_path(ny, nx);
        auto *g_ptr = &floor.get_grid(pos_path);

        if (in_bounds(&floor, ny, nx) && is_cave_empty_bold(player_ptr, ny, nx) && !g_ptr->is_object() && !pattern_tile(&floor, ny, nx)) {
            ty = ny;
            tx = nx;
        }
    }

    floor.get_grid(pos).m_idx = 0;
    floor.get_grid({ ty, tx }).m_idx = m_idx;
    monster.fy = ty;
    monster.fx = tx;
    (void)set_monster_csleep(player_ptr, m_idx, 0);
    update_monster(player_ptr, m_idx, true);
    lite_spot(player_ptr, target_row, target_col);
    lite_spot(player_ptr, ty, tx);
    if (monster.get_monrace().brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    if (monster.ml) {
        if (!player_ptr->effects()->hallucination()->is_hallucinated()) {
            monster_race_track(player_ptr, monster.ap_r_idx);
        }

        health_track(player_ptr, m_idx);
    }

    return true;
}
