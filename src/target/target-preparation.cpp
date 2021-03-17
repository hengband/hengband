#include <utility>
#include <vector>

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
#include "system/object-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-preparation.h"
#include "target/target-types.h"
#include "util/bit-flags-calculator.h"
#include "util/sort.h"
#include "window/main-window-util.h"

#include <algorithm>

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
bool target_able(player_type *creature_ptr, MONSTER_IDX m_idx)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    monster_type *m_ptr = &floor_ptr->m_list[m_idx];
    if (!monster_is_valid(m_ptr))
        return FALSE;

    if (creature_ptr->image)
        return FALSE;

    if (!m_ptr->ml)
        return FALSE;

    if (creature_ptr->riding && (creature_ptr->riding == m_idx))
        return TRUE;

    if (!projectable(creature_ptr, creature_ptr->y, creature_ptr->x, m_ptr->fy, m_ptr->fx))
        return FALSE;

    return TRUE;
}

/*
 * Determine if a given location is "interesting"
 */
static bool target_set_accept(player_type *creature_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!(in_bounds(floor_ptr, y, x)))
        return FALSE;

    if (player_bold(creature_ptr, y, x))
        return TRUE;

    if (creature_ptr->image)
        return FALSE;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (g_ptr->m_idx) {
        monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
        if (m_ptr->ml)
            return TRUE;
    }

    OBJECT_IDX next_o_idx = 0;
    for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if (o_ptr->marked & OM_FOUND)
            return TRUE;
    }

    if (g_ptr->info & (CAVE_MARK)) {
        if (g_ptr->info & CAVE_OBJECT)
            return TRUE;

        if (has_flag(f_info[get_feat_mimic(g_ptr)].flags, FF_NOTICE))
            return TRUE;
    }

    return FALSE;
}

/*!
 * @brief "interesting" な座標たちを ys, xs に返す。
 * @param creature_ptr
 * @param ys y座標たちを格納する配列 (POSITION 型)
 * @param xs x座標たちを格納する配列 (POSITION 型)
 * @param mode
 *
 * ys, xs は処理開始時にクリアされる。
 */
void target_set_prepare(player_type *creature_ptr, std::vector<POSITION> &ys, std::vector<POSITION> &xs, const BIT_FLAGS mode)
{
    POSITION min_hgt, max_hgt, min_wid, max_wid;
    if (mode & TARGET_KILL) {
        min_hgt = MAX((creature_ptr->y - get_max_range(creature_ptr)), 0);
        max_hgt = MIN((creature_ptr->y + get_max_range(creature_ptr)), creature_ptr->current_floor_ptr->height - 1);
        min_wid = MAX((creature_ptr->x - get_max_range(creature_ptr)), 0);
        max_wid = MIN((creature_ptr->x + get_max_range(creature_ptr)), creature_ptr->current_floor_ptr->width - 1);
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
            if (!target_set_accept(creature_ptr, y, x))
                continue;

            g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];
            if ((mode & (TARGET_KILL)) && !target_able(creature_ptr, g_ptr->m_idx))
                continue;

            if ((mode & (TARGET_KILL)) && !target_pet && is_pet(&creature_ptr->current_floor_ptr->m_list[g_ptr->m_idx]))
                continue;

            ys.emplace_back(y);
            xs.emplace_back(x);
        }
    }

    if (mode & (TARGET_KILL)) {
        ang_sort(creature_ptr, xs.data(), ys.data(), size(ys), ang_sort_comp_distance, ang_sort_swap_position);
    } else {
        ang_sort(creature_ptr, xs.data(), ys.data(), size(ys), ang_sort_comp_importance, ang_sort_swap_position);
    }

    // 乗っているモンスターがターゲットリストの先頭にならないようにする調整。

    if (creature_ptr->riding == 0 || !target_pet || (size(ys) <= 1) || !(mode & (TARGET_KILL)))
        return;

    // 0 番目と 1 番目を入れ替える。
    std::swap(ys[0], ys[1]);
    std::swap(xs[0], xs[1]);
}

void target_sensing_monsters_prepare(player_type *creature_ptr, std::vector<MONSTER_IDX> &monster_list)
{
    monster_list.clear();

    // 幻覚時は正常に感知できない
    if (creature_ptr->image)
        return;

    for (int i = 1; i < creature_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || !m_ptr->ml || is_pet(m_ptr))
            continue;

        // 感知魔法/スキルやESPで感知していない擬態モンスターはモンスター一覧に表示しない
        if (is_mimicry(m_ptr) && none_bits(m_ptr->mflag2, (MFLAG2_MARK | MFLAG2_SHOW)) && none_bits(m_ptr->mflag, MFLAG_ESP))
            continue;

        monster_list.push_back(i);
    }

    auto comp_importance = [floor_ptr = creature_ptr->current_floor_ptr](MONSTER_IDX idx1, MONSTER_IDX idx2) {
        auto m_ptr1 = &floor_ptr->m_list[idx1];
        auto m_ptr2 = &floor_ptr->m_list[idx2];
        auto ap_r_ptr1 = &r_info[m_ptr1->ap_r_idx];
        auto ap_r_ptr2 = &r_info[m_ptr2->ap_r_idx];

        /* Unique monsters first */
        if (any_bits(ap_r_ptr1->flags1, RF1_UNIQUE) != any_bits(ap_r_ptr2->flags1, RF1_UNIQUE))
            return any_bits(ap_r_ptr1->flags1, RF1_UNIQUE);

        /* Shadowers first (あやしい影) */
        if (any_bits(m_ptr1->mflag2, MFLAG2_KAGE) != any_bits(m_ptr2->mflag2, MFLAG2_KAGE))
            return any_bits(m_ptr1->mflag2, MFLAG2_KAGE);

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
