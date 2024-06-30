#include "monster/monster-compaction.h"
#include "core/stuff-handler.h"
#include "game-option/play-record-options.h"
#include "io/write-diary.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "tracking/health-bar-tracker.h"
#include "view/display-messages.h"

/*!
 * @brief モンスター情報を配列内移動する / Move an object from index i1 to index i2 in the object list
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i1 配列移動元添字
 * @param i2 配列移動先添字
 */
static void compact_monsters_aux(PlayerType *player_ptr, MONSTER_IDX i1, MONSTER_IDX i2)
{
    if (i1 == i2) {
        return;
    }

    auto *floor_ptr = player_ptr->current_floor_ptr;
    MonsterEntity *m_ptr;
    m_ptr = &floor_ptr->m_list[i1];

    POSITION y = m_ptr->fy;
    POSITION x = m_ptr->fx;
    Grid *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    g_ptr->m_idx = i2;

    for (const auto this_o_idx : m_ptr->hold_o_idx_list) {
        ItemEntity *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        o_ptr->held_m_idx = i2;
    }

    if (target_who == i1) {
        target_who = i2;
    }

    if (player_ptr->pet_t_m_idx == i1) {
        player_ptr->pet_t_m_idx = i2;
    }
    if (player_ptr->riding_t_m_idx == i1) {
        player_ptr->riding_t_m_idx = i2;
    }

    if (player_ptr->riding == i1) {
        player_ptr->riding = i2;
    }

    if (HealthBarTracker::get_instance().is_tracking(i1)) {
        health_track(player_ptr, i2);
    }

    if (m_ptr->is_pet()) {
        for (int i = 1; i < floor_ptr->m_max; i++) {
            MonsterEntity *m2_ptr = &floor_ptr->m_list[i];

            if (m2_ptr->parent_m_idx == i1) {
                m2_ptr->parent_m_idx = i2;
            }
        }
    }

    floor_ptr->m_list[i2] = floor_ptr->m_list[i1];
    floor_ptr->m_list[i1] = {};

    for (int i = 0; i < MAX_MTIMED; i++) {
        int mproc_idx = get_mproc_idx(floor_ptr, i1, i);
        if (mproc_idx >= 0) {
            floor_ptr->mproc_list[i][mproc_idx] = i2;
        }
    }
}

/*!
 * @brief モンスター情報配列を圧縮する / Compact and Reorder the monster list
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param size 圧縮後のモンスター件数目標
 * @details
 * This function can be very dangerous, use with caution!
 *
 * When actually "compacting" monsters, we base the saving throw
 * on a combination of monster level, distance from player, and
 * current "desperation".
 *
 * After "compacting" (if needed), we "reorder" the monsters into a more
 * compact order, and we reset the allocation info, and the "live" array.
 */
void compact_monsters(PlayerType *player_ptr, int size)
{
    if (size) {
        msg_print(_("モンスター情報を圧縮しています...", "Compacting monsters..."));
    }

    /* Compact at least 'size' objects */
    auto &floor = *player_ptr->current_floor_ptr;
    for (int num = 0, cnt = 1; num < size; cnt++) {
        int cur_lev = 5 * cnt;
        int cur_dis = 5 * (20 - cnt);
        for (MONSTER_IDX i = 1; i < floor.m_max; i++) {
            const auto &monster = floor.m_list[i];
            const auto &monrace = monster.get_monrace();
            if (!monster.is_valid()) {
                continue;
            }
            if (monrace.level > cur_lev) {
                continue;
            }
            if (i == player_ptr->riding) {
                continue;
            }
            if ((cur_dis > 0) && (monster.cdis < cur_dis)) {
                continue;
            }

            int chance = 90;
            if (monrace.misc_flags.has(MonsterMiscType::QUESTOR) && (cnt < 1000)) {
                chance = 100;
            }

            if (monrace.kind_flags.has(MonsterKindType::UNIQUE)) {
                chance = 100;
            }

            if (evaluate_percent(chance)) {
                continue;
            }

            if (record_named_pet && monster.is_named_pet()) {
                const auto m_name = monster_desc(player_ptr, &monster, MD_INDEF_VISIBLE);
                exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_COMPACT, m_name);
            }

            delete_monster_idx(player_ptr, i);
            num++;
        }
    }

    /* Excise dead monsters (backwards!) */
    for (MONSTER_IDX i = floor.m_max - 1; i >= 1; i--) {
        const auto &monster = floor.m_list[i];
        if (monster.is_valid()) {
            continue;
        }

        compact_monsters_aux(player_ptr, floor.m_max - 1, i);
        floor.m_max--;
    }
}
