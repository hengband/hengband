#include "target/target-preparation.h"
#include "floor/cave.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object/object-mark-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-types.h"
#include "util/bit-flags-calculator.h"
#include "util/sort.h"
#include "window/main-window-util.h"
#include <algorithm>
#include <utility>
#include <vector>

/*
 * Determine is a monster makes a reasonable target
 *
 * The concept of "targeting" was stolen from "Morgul" (?)
 *
 * The player can target any location, or any "target-able" monster.
 *
 * Currently, a monster is "target_able" if it is visible, and if
 * the player can hit it with a projection, and the player is not
 * hallucinating.  This allows use of "use closest target" macros.
 *
 * Future versions may restrict the ability to target "trappers"
 * and "mimics", but the semantics is a little bit weird.
 */
bool target_able(PlayerType *player_ptr, MONSTER_IDX m_idx)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    if (!monster_is_valid(m_ptr))
        return false;

    if (player_ptr->hallucinated)
        return false;

    if (!m_ptr->ml)
        return false;

    if (player_ptr->riding && (player_ptr->riding == m_idx))
        return true;

    if (!projectable(player_ptr, player_ptr->y, player_ptr->x, m_ptr->fy, m_ptr->fx))
        return false;

    return true;
}

/*
 * Determine if a given location is "interesting"
 */
static bool target_set_accept(PlayerType *player_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!(in_bounds(floor_ptr, y, x)))
        return false;

    if (player_bold(player_ptr, y, x))
        return true;

    if (player_ptr->hallucinated)
        return false;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->m_idx) {
        monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
        if (m_ptr->ml)
            return true;
    }

    for (const auto this_o_idx : g_ptr->o_idx_list) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if (o_ptr->marked & OM_FOUND)
            return true;
    }

    if (g_ptr->is_mark()) {
        if (g_ptr->is_object())
            return true;

        if (f_info[g_ptr->get_feat_mimic()].flags.has(FloorFeatureType::NOTICE))
            return true;
    }

    return false;
}

/*!
 * @brief "interesting" な座標たちを ys, xs に返す。
 * @param player_ptr
 * @param ys y座標たちを格納する配列 (POSITION 型)
 * @param xs x座標たちを格納する配列 (POSITION 型)
 * @param mode
 *
 * ys, xs は処理開始時にクリアされる。
 */
void target_set_prepare(PlayerType *player_ptr, std::vector<POSITION> &ys, std::vector<POSITION> &xs, const BIT_FLAGS mode)
{
    POSITION min_hgt, max_hgt, min_wid, max_wid;
    if (mode & TARGET_KILL) {
        min_hgt = std::max((player_ptr->y - get_max_range(player_ptr)), 0);
        max_hgt = std::min((player_ptr->y + get_max_range(player_ptr)), player_ptr->current_floor_ptr->height - 1);
        min_wid = std::max((player_ptr->x - get_max_range(player_ptr)), 0);
        max_wid = std::min((player_ptr->x + get_max_range(player_ptr)), player_ptr->current_floor_ptr->width - 1);
    } else {
        min_hgt = panel_row_min;
        max_hgt = panel_row_max;
        min_wid = panel_col_min;
        max_wid = panel_col_max;
    }

    ys.clear();
    xs.clear();

    for (POSITION y = min_hgt; y <= max_hgt; y++) {
        for (POSITION x = min_wid; x <= max_wid; x++) {
            grid_type *g_ptr;
            if (!target_set_accept(player_ptr, y, x))
                continue;

            g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
            if ((mode & (TARGET_KILL)) && !target_able(player_ptr, g_ptr->m_idx))
                continue;

            if ((mode & (TARGET_KILL)) && !target_pet && is_pet(&player_ptr->current_floor_ptr->m_list[g_ptr->m_idx]))
                continue;

            ys.emplace_back(y);
            xs.emplace_back(x);
        }
    }

    if (mode & (TARGET_KILL)) {
        ang_sort(player_ptr, xs.data(), ys.data(), size(ys), ang_sort_comp_distance, ang_sort_swap_position);
    } else {
        ang_sort(player_ptr, xs.data(), ys.data(), size(ys), ang_sort_comp_importance, ang_sort_swap_position);
    }

    // 乗っているモンスターがターゲットリストの先頭にならないようにする調整。

    if (player_ptr->riding == 0 || !target_pet || (size(ys) <= 1) || !(mode & (TARGET_KILL)))
        return;

    // 0 番目と 1 番目を入れ替える。
    std::swap(ys[0], ys[1]);
    std::swap(xs[0], xs[1]);
}

void target_sensing_monsters_prepare(PlayerType *player_ptr, std::vector<MONSTER_IDX> &monster_list)
{
    monster_list.clear();

    // 幻覚時は正常に感知できない
    if (player_ptr->hallucinated)
        return;

    for (MONSTER_IDX i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || !m_ptr->ml || is_pet(m_ptr))
            continue;

        // 感知魔法/スキルやESPで感知していない擬態モンスターはモンスター一覧に表示しない
        if (is_mimicry(m_ptr) && m_ptr->mflag2.has_none_of({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW }) && m_ptr->mflag.has_not(MonsterTemporaryFlagType::ESP))
            continue;

        monster_list.push_back(i);
    }

    auto comp_importance = [floor_ptr = player_ptr->current_floor_ptr](MONSTER_IDX idx1, MONSTER_IDX idx2) {
        auto m_ptr1 = &floor_ptr->m_list[idx1];
        auto m_ptr2 = &floor_ptr->m_list[idx2];
        auto ap_r_ptr1 = &r_info[m_ptr1->ap_r_idx];
        auto ap_r_ptr2 = &r_info[m_ptr2->ap_r_idx];

        /* Unique monsters first */
        if (any_bits(ap_r_ptr1->flags1, RF1_UNIQUE) != any_bits(ap_r_ptr2->flags1, RF1_UNIQUE))
            return any_bits(ap_r_ptr1->flags1, RF1_UNIQUE);

        /* Shadowers first (あやしい影) */
        if (m_ptr1->mflag2.has(MonsterConstantFlagType::KAGE) != m_ptr2->mflag2.has(MonsterConstantFlagType::KAGE))
            return m_ptr1->mflag2.has(MonsterConstantFlagType::KAGE);

        /* Unknown monsters first */
        if ((ap_r_ptr1->r_tkills == 0) != (ap_r_ptr2->r_tkills == 0))
            return (ap_r_ptr1->r_tkills == 0);

        /* Higher level monsters first (if known) */
        if (ap_r_ptr1->r_tkills && ap_r_ptr2->r_tkills && ap_r_ptr1->level != ap_r_ptr2->level)
            return ap_r_ptr1->level > ap_r_ptr2->level;

        /* Sort by index if all conditions are same */
        return m_ptr1->ap_r_idx > m_ptr2->ap_r_idx;
    };

    std::sort(monster_list.begin(), monster_list.end(), comp_importance);
}
