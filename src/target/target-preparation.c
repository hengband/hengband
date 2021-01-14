#include "target/target-preparation.h"
#include "floor/cave.h"
#include "game-option/input-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "object/object-mark-types.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "target/projection-path-calculator.h"
#include "target/target-types.h"
#include "util/bit-flags-calculator.h"
#include "util/sort.h"
#include "window/main-window-util.h"

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

/*
 * Prepare the "temp" array for "target_set"
 *
 * Return the number of target_able monsters in the set.
 */
void target_set_prepare(player_type *creature_ptr, BIT_FLAGS mode)
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

    tmp_pos.n = 0;
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

            tmp_pos.x[tmp_pos.n] = x;
            tmp_pos.y[tmp_pos.n] = y;
            tmp_pos.n++;
        }
    }

    if (mode & (TARGET_KILL)) {
        ang_sort(creature_ptr, tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_distance, ang_sort_swap_distance);
    } else {
        ang_sort(creature_ptr, tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_importance, ang_sort_swap_distance);
    }

    if (creature_ptr->riding == 0 || !target_pet || (tmp_pos.n <= 1) || !(mode & (TARGET_KILL)))
        return;

    POSITION tmp = tmp_pos.y[0];
    tmp_pos.y[0] = tmp_pos.y[1];
    tmp_pos.y[1] = tmp;
    tmp = tmp_pos.x[0];
    tmp_pos.x[0] = tmp_pos.x[1];
    tmp_pos.x[1] = tmp;
}
