#include "spell-kind/spells-fetcher.h"
#include "core/stuff-handler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/projection-path-calculator.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "timed-effect/timed-effects.h"
#include "tracking/lore-tracker.h"
#include "view/display-messages.h"

/*!
 * @brief アイテム引き寄せ処理 /
 * Fetch an item (teleport it right underneath the caster)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 魔法の発動方向
 * @param wgt 許容重量
 * @param require_los 射線の通りを要求するならばTRUE
 */
void fetch_item(PlayerType *player_ptr, const Direction &dir, WEIGHT wgt, bool require_los)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    if (!floor.get_grid(p_pos).o_idx_list.empty()) {
        msg_print(_("自分の足の下にある物は取れません。", "You can't fetch when you're already standing on something."));
        return;
    }

    Grid *grid_ptr;
    const auto &system = AngbandSystem::get_instance();
    if (dir.is_target_okay()) {
        const auto pos = dir.get_target_position(p_pos);
        if (Grid::calc_distance(p_pos, pos) > system.get_max_range()) {
            msg_print(_("そんなに遠くにある物は取れません！", "You can't fetch something that far away!"));
            return;
        }

        grid_ptr = &floor.get_grid(pos);
        if (grid_ptr->o_idx_list.empty()) {
            msg_print(_("そこには何もありません。", "There is no object there."));
            return;
        }

        if (grid_ptr->is_icky()) {
            msg_print(_("アイテムがコントロールを外れて落ちた。", "The item slips from your control."));
            return;
        }

        if (require_los) {
            if (!floor.has_los_at(pos)) {
                msg_print(_("そこはあなたの視界に入っていません。", "You have no direct line of sight to that location."));
                return;
            } else if (!projectable(player_ptr, p_pos, pos)) {
                msg_print(_("そこは壁の向こうです。", "You have no direct line of sight to that location."));
                return;
            }
        }
    } else {
        auto pos = p_pos;
        auto is_first_loop = true;
        grid_ptr = &floor.get_grid(pos);
        while (is_first_loop || grid_ptr->o_idx_list.empty()) {
            is_first_loop = false;
            pos += dir.vec();
            grid_ptr = &floor.get_grid(pos);
            if ((Grid::calc_distance(p_pos, pos) > system.get_max_range())) {
                return;
            }

            if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECT)) {
                return;
            }
        }
    }

    auto &item = *floor.o_list[grid_ptr->o_idx_list.front()];
    if (item.weight > wgt) {
        msg_print(_("そのアイテムは重過ぎます。", "The object is too heavy."));
        return;
    }

    const auto item_idx = grid_ptr->o_idx_list.front();
    grid_ptr->o_idx_list.pop_front();
    floor.get_grid(p_pos).o_idx_list.add(floor, item_idx); /* 'move' it */
    item.set_position(p_pos);

    const auto item_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
    msg_format(_("%s^があなたの足元に飛んできた。", "%s^ flies through the air to your feet."), item_name.data());
    note_spot(player_ptr, p_pos);
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::MAP);
}

bool fetch_monster(PlayerType *player_ptr)
{
    const auto pos = target_set(player_ptr, TARGET_KILL).get_position();
    if (!pos) {
        return false;
    }

    auto &floor = *player_ptr->current_floor_ptr;
    const auto m_idx = floor.get_grid(*pos).m_idx;
    if (!is_monster(m_idx)) {
        return false;
    }
    auto &monster = floor.m_list[m_idx];
    if (monster.is_riding()) {
        return false;
    }
    if (!floor.has_los_at(*pos)) {
        return false;
    }

    const auto p_pos = player_ptr->get_position();
    if (!projectable(player_ptr, p_pos, *pos)) {
        return false;
    }

    const auto m_name = monster_desc(player_ptr, monster, 0);
    msg_print(_("{}を引き戻した。", "You pull back {}."), m_name);
    ProjectionPath path_g(player_ptr, AngbandSystem::get_instance().get_max_range(), *pos, p_pos, 0);
    Pos2D pos_target = *pos;
    for (const auto &pos_path : path_g) {
        const auto &grid = floor.get_grid(pos_path);
        if (floor.contains(pos_path) && floor.is_empty_at(pos_path) && (pos_path != p_pos) && !grid.is_object() && !pattern_tile(floor, pos_path.y, pos_path.x)) {
            pos_target = pos_path;
        }
    }

    floor.get_grid(*pos).m_idx = 0;
    floor.get_grid(pos_target).m_idx = m_idx;
    monster.set_position(pos_target);
    (void)set_monster_csleep(player_ptr, m_idx, 0);
    update_monster(player_ptr, m_idx, true);
    lite_spot(player_ptr, *pos);
    lite_spot(player_ptr, pos_target);
    if (monster.get_monrace().brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }

    if (monster.ml) {
        if (!player_ptr->effects()->hallucination().is_hallucinated()) {
            LoreTracker::get_instance().set_trackee(monster.ap_r_idx);
        }

        health_track(player_ptr, m_idx);
    }

    return true;
}
