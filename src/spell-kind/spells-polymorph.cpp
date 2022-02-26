#include "spell-kind/spells-polymorph.h"
#include "core/stuff-handler.h"
#include "floor/floor-object.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status.h"
#include "monster/monster-util.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief 変身処理向けにモンスターの近隣レベル帯モンスターを返す /
 * Helper function -- return a "nearby" race for polymorphing
 * @param floor_ptr 配置するフロアの参照ポインタ
 * @param r_idx 基準となるモンスター種族ID
 * @return 変更先のモンスター種族ID
 * @details
 * Note that this function is one of the more "dangerous" ones...
 */
static MONRACE_IDX poly_r_idx(PlayerType *player_ptr, MONRACE_IDX r_idx)
{
    auto *r_ptr = &r_info[r_idx];
    if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE) || any_bits(r_ptr->flags1, RF1_QUESTOR)) {
        return r_idx;
    }

    DEPTH lev1 = r_ptr->level - ((randint1(20) / randint1(9)) + 1);
    DEPTH lev2 = r_ptr->level + ((randint1(20) / randint1(9)) + 1);
    MONRACE_IDX r;
    for (int i = 0; i < 1000; i++) {
        r = get_mon_num(player_ptr, 0, (player_ptr->current_floor_ptr->dun_level + r_ptr->level) / 2 + 5, 0);
        if (!r) {
            break;
        }

        r_ptr = &r_info[r];
        if (r_ptr->kind_flags.has(MonsterKindType::UNIQUE)) {
            continue;
        }
        if ((r_ptr->level < lev1) || (r_ptr->level > lev2)) {
            continue;
        }

        r_idx = r;
        break;
    }

    return r_idx;
}

/*!
 * @brief 指定座標にいるモンスターを変身させる /
 * Helper function -- return a "nearby" race for polymorphing
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 指定のY座標
 * @param x 指定のX座標
 * @return 実際に変身したらTRUEを返す
 */
bool polymorph_monster(PlayerType *player_ptr, POSITION y, POSITION x)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    auto *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    MONRACE_IDX new_r_idx;
    MONRACE_IDX old_r_idx = m_ptr->r_idx;
    bool targeted = target_who == g_ptr->m_idx;
    bool health_tracked = player_ptr->health_who == g_ptr->m_idx;

    if (floor_ptr->inside_arena || player_ptr->phase_out) {
        return false;
    }
    if ((player_ptr->riding == g_ptr->m_idx) || m_ptr->mflag2.has(MonsterConstantFlagType::KAGE)) {
        return false;
    }

    monster_type back_m = *m_ptr;
    new_r_idx = poly_r_idx(player_ptr, old_r_idx);
    if (new_r_idx == old_r_idx) {
        return false;
    }

    bool preserve_hold_objects = !back_m.hold_o_idx_list.empty();

    BIT_FLAGS mode = 0L;
    if (is_friendly(m_ptr)) {
        mode |= PM_FORCE_FRIENDLY;
    }
    if (is_pet(m_ptr)) {
        mode |= PM_FORCE_PET;
    }
    if (m_ptr->mflag2.has(MonsterConstantFlagType::NOPET)) {
        mode |= PM_NO_PET;
    }

    m_ptr->hold_o_idx_list.clear();
    delete_monster_idx(player_ptr, g_ptr->m_idx);
    bool polymorphed = false;
    if (place_monster_aux(player_ptr, 0, y, x, new_r_idx, mode)) {
        floor_ptr->m_list[hack_m_idx_ii].nickname = back_m.nickname;
        floor_ptr->m_list[hack_m_idx_ii].parent_m_idx = back_m.parent_m_idx;
        floor_ptr->m_list[hack_m_idx_ii].hold_o_idx_list = back_m.hold_o_idx_list;
        polymorphed = true;
    } else {
        if (place_monster_aux(player_ptr, 0, y, x, old_r_idx, (mode | PM_NO_KAGE | PM_IGNORE_TERRAIN))) {
            floor_ptr->m_list[hack_m_idx_ii] = back_m;
            mproc_init(floor_ptr);
        } else {
            preserve_hold_objects = false;
        }
    }

    if (preserve_hold_objects) {
        for (const auto this_o_idx : back_m.hold_o_idx_list) {
            auto *o_ptr = &floor_ptr->o_list[this_o_idx];
            o_ptr->held_m_idx = hack_m_idx_ii;
        }
    } else {
        for (auto it = back_m.hold_o_idx_list.begin(); it != back_m.hold_o_idx_list.end();) {
            OBJECT_IDX this_o_idx = *it++;
            delete_object_idx(player_ptr, this_o_idx);
        }
    }

    if (targeted) {
        target_who = hack_m_idx_ii;
    }
    if (health_tracked) {
        health_track(player_ptr, hack_m_idx_ii);
    }
    return polymorphed;
}
