#include "monster-floor/monster-remover.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"

/*!
 * @brief モンスター配列からモンスターを消去する / Delete a monster by index.
 * @param i 消去するモンスターのID
 * @details
 * モンスターを削除するとそのモンスターが拾っていたアイテムも同時に削除される。 /
 * When a monster is deleted, all of its objects are deleted.
 */
void delete_monster_idx(player_type *player_ptr, MONSTER_IDX i)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[i];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;

    real_r_ptr(m_ptr)->cur_num--;
    if (r_ptr->flags2 & (RF2_MULTIPLY))
        floor_ptr->num_repro--;

    if (monster_csleep_remaining(m_ptr))
        (void)set_monster_csleep(player_ptr, i, 0);
    if (monster_fast_remaining(m_ptr))
        (void)set_monster_fast(player_ptr, i, 0);
    if (monster_slow_remaining(m_ptr))
        (void)set_monster_slow(player_ptr, i, 0);
    if (monster_stunned_remaining(m_ptr))
        (void)set_monster_stunned(player_ptr, i, 0);
    if (monster_confused_remaining(m_ptr))
        (void)set_monster_confused(player_ptr, i, 0);
    if (monster_fear_remaining(m_ptr))
        (void)set_monster_monfear(player_ptr, i, 0);
    if (monster_invulner_remaining(m_ptr))
        (void)set_monster_invulner(player_ptr, i, 0, false);

    if (i == target_who)
        target_who = 0;

    if (i == player_ptr->health_who)
        health_track(player_ptr, 0);

    if (player_ptr->pet_t_m_idx == i)
        player_ptr->pet_t_m_idx = 0;
    if (player_ptr->riding_t_m_idx == i)
        player_ptr->riding_t_m_idx = 0;
    if (player_ptr->riding == i)
        player_ptr->riding = 0;

    floor_ptr->grid_array[y][x].m_idx = 0;
    for (auto it = m_ptr->hold_o_idx_list.begin(); it != m_ptr->hold_o_idx_list.end();) {
        const OBJECT_IDX this_o_idx = *it++;
        delete_object_idx(player_ptr, this_o_idx);
    }

    // 召喚元のモンスターが消滅した時は、召喚されたモンスターのparent_m_idxが
    // 召喚されたモンスター自身のm_idxを指すようにする
    for (MONSTER_IDX child_m_idx = 1; child_m_idx < floor_ptr->m_max; child_m_idx++) {
        monster_type *child_m_ptr = &floor_ptr->m_list[child_m_idx];
        if (child_m_ptr->r_idx && child_m_ptr->parent_m_idx == i) {
            child_m_ptr->parent_m_idx = child_m_idx;
        }
    }

    *m_ptr = {};
    floor_ptr->m_cnt--;
    lite_spot(player_ptr, y, x);
    if (r_ptr->flags7 & (RF7_LITE_MASK | RF7_DARK_MASK)) {
        player_ptr->update |= (PU_MON_LITE);
    }
}

/*!
 * @brief プレイヤーのフロア離脱に伴う全モンスター配列の消去 / Delete/Remove all the monsters when the player leaves the level
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * This is an efficient method of simulating multiple calls to the
 * "delete_monster()" function, with no visual effects.
 */
void wipe_monsters_list(player_type *player_ptr)
{
    if (!r_info[MON_BANORLUPART].max_num) {
        if (r_info[MON_BANOR].max_num) {
            r_info[MON_BANOR].max_num = 0;
            r_info[MON_BANOR].r_pkills++;
            r_info[MON_BANOR].r_akills++;
            if (r_info[MON_BANOR].r_tkills < MAX_SHORT)
                r_info[MON_BANOR].r_tkills++;
        }

        if (r_info[MON_LUPART].max_num) {
            r_info[MON_LUPART].max_num = 0;
            r_info[MON_LUPART].r_pkills++;
            r_info[MON_LUPART].r_akills++;
            if (r_info[MON_LUPART].r_tkills < MAX_SHORT)
                r_info[MON_LUPART].r_tkills++;
        }
    }

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = floor_ptr->m_max - 1; i >= 1; i--) {
        monster_type *m_ptr = &floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;

        floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].m_idx = 0;
        *m_ptr = {};
    }

    /*
     * Wiping racial counters of all monsters and incrementing of racial
     * counters of monsters in party_mon[] are required to prevent multiple
     * generation of unique monster who is the minion of player.
     */
    for (auto &r_ref : r_info)
        r_ref.cur_num = 0;

    floor_ptr->m_max = 1;
    floor_ptr->m_cnt = 0;
    for (int i = 0; i < MAX_MTIMED; i++)
        floor_ptr->mproc_max[i] = 0;

    floor_ptr->num_repro = 0;
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
void delete_monster(player_type *player_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x))
        return;

    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->m_idx)
        delete_monster_idx(player_ptr, g_ptr->m_idx);
}
