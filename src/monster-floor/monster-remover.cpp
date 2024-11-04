#include "monster-floor/monster-remover.h"
#include "core/stuff-handler.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "monster-race/race-brightness-mask.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "system/enums/monrace/monrace-id.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "tracking/health-bar-tracker.h"

/*!
 * @brief モンスター配列からモンスターを消去する / Delete a monster by index.
 * @param i 消去するモンスターのID
 * @details
 * モンスターを削除するとそのモンスターが拾っていたアイテムも同時に削除される。 /
 * When a monster is deleted, all of its objects are deleted.
 */
void delete_monster_idx(PlayerType *player_ptr, MONSTER_IDX i)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *m_ptr = &floor_ptr->m_list[i];
    auto *r_ptr = &m_ptr->get_monrace();

    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;

    m_ptr->get_real_monrace().decrement_current_numbers();
    if (r_ptr->misc_flags.has(MonsterMiscType::MULTIPLY)) {
        floor_ptr->num_repro--;
    }

    if (m_ptr->is_asleep()) {
        (void)set_monster_csleep(player_ptr, i, 0);
    }
    if (m_ptr->is_accelerated()) {
        (void)set_monster_fast(player_ptr, i, 0);
    }
    if (m_ptr->is_decelerated()) {
        (void)set_monster_slow(player_ptr, i, 0);
    }
    if (m_ptr->is_stunned()) {
        (void)set_monster_stunned(player_ptr, i, 0);
    }
    if (m_ptr->is_confused()) {
        (void)set_monster_confused(player_ptr, i, 0);
    }
    if (m_ptr->is_fearful()) {
        (void)set_monster_monfear(player_ptr, i, 0);
    }
    if (m_ptr->is_invulnerable()) {
        (void)set_monster_invulner(player_ptr, i, 0, false);
    }

    if (i == target_who) {
        target_who = 0;
    }

    if (HealthBarTracker::get_instance().is_tracking(i)) {
        health_track(player_ptr, 0);
    }

    if (player_ptr->pet_t_m_idx == i) {
        player_ptr->pet_t_m_idx = 0;
    }
    if (player_ptr->riding_t_m_idx == i) {
        player_ptr->riding_t_m_idx = 0;
    }
    if (m_ptr->is_riding()) { // player_ptr->riding == i のままの方がいい？
        player_ptr->ride_monster(0);
    }

    floor_ptr->grid_array[y][x].m_idx = 0;
    for (auto it = m_ptr->hold_o_idx_list.begin(); it != m_ptr->hold_o_idx_list.end();) {
        const OBJECT_IDX this_o_idx = *it++;
        delete_object_idx(player_ptr, this_o_idx);
    }

    // 召喚元のモンスターが消滅した時は、召喚されたモンスターのparent_m_idxが
    // 召喚されたモンスター自身のm_idxを指すようにする
    for (MONSTER_IDX child_m_idx = 1; child_m_idx < floor_ptr->m_max; child_m_idx++) {
        auto &child_monster = floor_ptr->m_list[child_m_idx];
        if (child_monster.is_valid() && child_monster.parent_m_idx == i) {
            child_monster.parent_m_idx = child_m_idx;
        }
    }

    *m_ptr = {};
    floor_ptr->m_cnt--;
    lite_spot(player_ptr, y, x);
    if (r_ptr->brightness_flags.has_any_of(ld_mask)) {
        RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::MONSTER_LITE);
    }
}

/*!
 * @brief プレイヤーのフロア離脱に伴う全モンスター配列の消去
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details 視覚効果なしでdelete_monster() をフロア全体に対して呼び出す.
 */
void wipe_monsters_list(PlayerType *player_ptr)
{
    auto &monraces = MonraceList::get_instance();
    monraces.defeat_separated_uniques();
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto i = floor.m_max - 1; i >= 1; i--) {
        auto &monster = floor.m_list[i];
        if (!monster.is_valid()) {
            continue;
        }

        floor.grid_array[monster.fy][monster.fx].m_idx = 0;
        monster = {};
    }

    monraces.reset_current_numbers();
    floor.m_max = 1;
    floor.m_cnt = 0;
    floor.reset_mproc_max();
    floor.num_repro = 0;
    target_who = 0;
    player_ptr->pet_t_m_idx = 0;
    player_ptr->riding_t_m_idx = 0;
    health_track(player_ptr, 0);
}

/*!
 * @brief 指定位置に存在するモンスターを削除する / Delete the monster, if any, at a given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param x 削除位置x座標
 * @param y 削除位置y座標
 */
void delete_monster(PlayerType *player_ptr, POSITION y, POSITION x)
{
    Grid *g_ptr;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x)) {
        return;
    }

    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->has_monster()) {
        delete_monster_idx(player_ptr, g_ptr->m_idx);
    }
}
