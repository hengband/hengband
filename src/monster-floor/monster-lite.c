#include "monster-floor/monster-lite.h"
#include "core/player-update-types.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "monster-floor/monster-lite-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-status.h"
#include "player/special-defense-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief モンスターによる光量状態更新 / Add a square to the changes array
 * @param subject_ptr 主観となるクリーチャーの参照ポインタ
 * @param y Y座標
 * @param x X座標
 */
static void update_monster_lite(player_type *subject_ptr, const POSITION y, const POSITION x, monster_lite_type *ml_ptr)
{
    grid_type *g_ptr;
    int dpf, d;
    POSITION midpoint;
    g_ptr = &subject_ptr->current_floor_ptr->grid_array[y][x];
    if ((g_ptr->info & (CAVE_MNLT | CAVE_VIEW)) != CAVE_VIEW)
        return;

    if (!cave_los_grid(g_ptr)) {
        if (((y < subject_ptr->y) && (y > ml_ptr->mon_fy)) || ((y > subject_ptr->y) && (y < ml_ptr->mon_fy))) {
            dpf = subject_ptr->y - ml_ptr->mon_fy;
            d = y - ml_ptr->mon_fy;
            midpoint = ml_ptr->mon_fx + ((subject_ptr->x - ml_ptr->mon_fx) * ABS(d)) / ABS(dpf);
            if (x < midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x + 1))
                    return;
            } else if (x > midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x - 1))
                    return;
            } else if (ml_ptr->mon_invis)
                return;
        }

        if (((x < subject_ptr->x) && (x > ml_ptr->mon_fx)) || ((x > subject_ptr->x) && (x < ml_ptr->mon_fx))) {
            dpf = subject_ptr->x - ml_ptr->mon_fx;
            d = x - ml_ptr->mon_fx;
            midpoint = ml_ptr->mon_fy + ((subject_ptr->y - ml_ptr->mon_fy) * ABS(d)) / ABS(dpf);
            if (y < midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y + 1, x))
                    return;
            } else if (y > midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y - 1, x))
                    return;
            } else if (ml_ptr->mon_invis)
                return;
        }
    }

    if (!(g_ptr->info & CAVE_MNDK)) {
        tmp_pos.x[tmp_pos.n] = x;
        tmp_pos.y[tmp_pos.n] = y;
        tmp_pos.n++;
    } else {
        g_ptr->info &= ~(CAVE_MNDK);
    }

    g_ptr->info |= CAVE_MNLT;
}

/*
 * Add a square to the changes array
 */
static void update_monster_dark(player_type *subject_ptr, const POSITION y, const POSITION x, monster_lite_type *ml_ptr)
{
    grid_type *g_ptr;
    int midpoint, dpf, d;
    g_ptr = &subject_ptr->current_floor_ptr->grid_array[y][x];
    if ((g_ptr->info & (CAVE_LITE | CAVE_MNLT | CAVE_MNDK | CAVE_VIEW)) != CAVE_VIEW)
        return;

    if (!cave_los_grid(g_ptr) && !cave_has_flag_grid(g_ptr, FF_PROJECT)) {
        if (((y < subject_ptr->y) && (y > ml_ptr->mon_fy)) || ((y > subject_ptr->y) && (y < ml_ptr->mon_fy))) {
            dpf = subject_ptr->y - ml_ptr->mon_fy;
            d = y - ml_ptr->mon_fy;
            midpoint = ml_ptr->mon_fx + ((subject_ptr->x - ml_ptr->mon_fx) * ABS(d)) / ABS(dpf);
            if (x < midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x + 1) && !cave_has_flag_bold(subject_ptr->current_floor_ptr, y, x + 1, FF_PROJECT))
                    return;
            } else if (x > midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y, x - 1) && !cave_has_flag_bold(subject_ptr->current_floor_ptr, y, x - 1, FF_PROJECT))
                    return;
            } else if (ml_ptr->mon_invis)
                return;
        }

        if (((x < subject_ptr->x) && (x > ml_ptr->mon_fx)) || ((x > subject_ptr->x) && (x < ml_ptr->mon_fx))) {
            dpf = subject_ptr->x - ml_ptr->mon_fx;
            d = x - ml_ptr->mon_fx;
            midpoint = ml_ptr->mon_fy + ((subject_ptr->y - ml_ptr->mon_fy) * ABS(d)) / ABS(dpf);
            if (y < midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y + 1, x) && !cave_has_flag_bold(subject_ptr->current_floor_ptr, y + 1, x, FF_PROJECT))
                    return;
            } else if (y > midpoint) {
                if (!cave_los_bold(subject_ptr->current_floor_ptr, y - 1, x) && !cave_has_flag_bold(subject_ptr->current_floor_ptr, y - 1, x, FF_PROJECT))
                    return;
            } else if (ml_ptr->mon_invis)
                return;
        }
    }

    tmp_pos.x[tmp_pos.n] = x;
    tmp_pos.y[tmp_pos.n] = y;
    tmp_pos.n++;
    g_ptr->info |= CAVE_MNDK;
}

/*
 * todo player-status からのみ呼ばれている。しかしあちらは行数が酷いので要調整
 * Update squares illuminated or darkened by monsters.
 * The CAVE_TEMP and CAVE_XTRA flag are used to store the state during the
 * updating.  Only squares in view of the player, whos state
 * changes are drawn via lite_spot().
 */
void update_mon_lite(player_type *subject_ptr)
{
    void (*add_mon_lite)(player_type *, const POSITION, const POSITION, monster_lite_type *);
    int dis_lim = ((d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS) && !subject_ptr->see_nocto) ? (MAX_SIGHT / 2 + 1) : (MAX_SIGHT + 3);
    floor_type *floor_ptr = subject_ptr->current_floor_ptr;
    for (int i = 0; i < floor_ptr->mon_lite_n; i++) {
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[floor_ptr->mon_lite_y[i]][floor_ptr->mon_lite_x[i]];
        g_ptr->info |= (g_ptr->info & CAVE_MNLT) ? CAVE_TEMP : CAVE_XTRA;
        g_ptr->info &= ~(CAVE_MNLT | CAVE_MNDK);
    }

    tmp_pos.n = 0;
    if (!current_world_ptr->timewalk_m_idx) {
        monster_type *m_ptr;
        monster_race *r_ptr;
        for (int i = 1; i < floor_ptr->m_max; i++) {
            m_ptr = &floor_ptr->m_list[i];
            r_ptr = &r_info[m_ptr->r_idx];
            if (!monster_is_valid(m_ptr) || (m_ptr->cdis > dis_lim))
                continue;

            int rad = 0;
            if (r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_SELF_LITE_1))
                rad++;

            if (r_ptr->flags7 & (RF7_HAS_LITE_2 | RF7_SELF_LITE_2))
                rad += 2;

            if (r_ptr->flags7 & (RF7_HAS_DARK_1 | RF7_SELF_DARK_1))
                rad--;

            if (r_ptr->flags7 & (RF7_HAS_DARK_2 | RF7_SELF_DARK_2))
                rad -= 2;

            if (!rad)
                continue;

            int f_flag;
            if (rad > 0) {
                if (!(r_ptr->flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2))
                    && (monster_csleep_remaining(m_ptr) || (!floor_ptr->dun_level && is_daytime()) || subject_ptr->phase_out))
                    continue;

                if (d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS)
                    rad = 1;

                add_mon_lite = update_monster_lite;
                f_flag = FF_LOS;
            } else {
                if (!(r_ptr->flags7 & (RF7_SELF_DARK_1 | RF7_SELF_DARK_2)) && (monster_csleep_remaining(m_ptr) || (!floor_ptr->dun_level && !is_daytime())))
                    continue;

                add_mon_lite = update_monster_dark;
                f_flag = FF_PROJECT;
                rad = -rad;
            }

            monster_lite_type tmp_ml;
            monster_lite_type *ml_ptr = initialize_monster_lite_type(floor_ptr, &tmp_ml, m_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx, ml_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx, ml_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx, ml_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx + 1, ml_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx - 1, ml_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx + 1, ml_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx - 1, ml_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx + 1, ml_ptr);
            add_mon_lite(subject_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx - 1, ml_ptr);
            if (rad < 2)
                continue;

            grid_type *g_ptr;
            if (cave_has_flag_bold(subject_ptr->current_floor_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx, f_flag)) {
                add_mon_lite(subject_ptr, ml_ptr->mon_fy + 2, ml_ptr->mon_fx + 1, ml_ptr);
                add_mon_lite(subject_ptr, ml_ptr->mon_fy + 2, ml_ptr->mon_fx, ml_ptr);
                add_mon_lite(subject_ptr, ml_ptr->mon_fy + 2, ml_ptr->mon_fx - 1, ml_ptr);
                g_ptr = &floor_ptr->grid_array[ml_ptr->mon_fy + 2][ml_ptr->mon_fx];
                if ((rad == 3) && cave_has_flag_grid(g_ptr, f_flag)) {
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy + 3, ml_ptr->mon_fx + 1, ml_ptr);
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy + 3, ml_ptr->mon_fx, ml_ptr);
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy + 3, ml_ptr->mon_fx - 1, ml_ptr);
                }
            }

            if (cave_has_flag_bold(subject_ptr->current_floor_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx, f_flag)) {
                add_mon_lite(subject_ptr, ml_ptr->mon_fy - 2, ml_ptr->mon_fx + 1, ml_ptr);
                add_mon_lite(subject_ptr, ml_ptr->mon_fy - 2, ml_ptr->mon_fx, ml_ptr);
                add_mon_lite(subject_ptr, ml_ptr->mon_fy - 2, ml_ptr->mon_fx - 1, ml_ptr);
                g_ptr = &floor_ptr->grid_array[ml_ptr->mon_fy - 2][ml_ptr->mon_fx];
                if ((rad == 3) && cave_has_flag_grid(g_ptr, f_flag)) {
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy - 3, ml_ptr->mon_fx + 1, ml_ptr);
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy - 3, ml_ptr->mon_fx, ml_ptr);
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy - 3, ml_ptr->mon_fx - 1, ml_ptr);
                }
            }

            if (cave_has_flag_bold(subject_ptr->current_floor_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx + 1, f_flag)) {
                add_mon_lite(subject_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx + 2, ml_ptr);
                add_mon_lite(subject_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx + 2, ml_ptr);
                add_mon_lite(subject_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx + 2, ml_ptr);
                g_ptr = &floor_ptr->grid_array[ml_ptr->mon_fy][ml_ptr->mon_fx + 2];
                if ((rad == 3) && cave_has_flag_grid(g_ptr, f_flag)) {
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx + 3, ml_ptr);
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx + 3, ml_ptr);
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx + 3, ml_ptr);
                }
            }

            if (cave_has_flag_bold(subject_ptr->current_floor_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx - 1, f_flag)) {
                add_mon_lite(subject_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx - 2, ml_ptr);
                add_mon_lite(subject_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx - 2, ml_ptr);
                add_mon_lite(subject_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx - 2, ml_ptr);
                g_ptr = &floor_ptr->grid_array[ml_ptr->mon_fy][ml_ptr->mon_fx - 2];
                if ((rad == 3) && cave_has_flag_grid(g_ptr, f_flag)) {
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx - 3, ml_ptr);
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx - 3, ml_ptr);
                    add_mon_lite(subject_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx - 3, ml_ptr);
                }
            }

            if (rad != 3)
                continue;

            if (cave_has_flag_bold(subject_ptr->current_floor_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx + 1, f_flag))
                add_mon_lite(subject_ptr, ml_ptr->mon_fy + 2, ml_ptr->mon_fx + 2, ml_ptr);

            if (cave_has_flag_bold(subject_ptr->current_floor_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx - 1, f_flag))
                add_mon_lite(subject_ptr, ml_ptr->mon_fy + 2, ml_ptr->mon_fx - 2, ml_ptr);

            if (cave_has_flag_bold(subject_ptr->current_floor_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx + 1, f_flag))
                add_mon_lite(subject_ptr, ml_ptr->mon_fy - 2, ml_ptr->mon_fx + 2, ml_ptr);

            if (cave_has_flag_bold(subject_ptr->current_floor_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx - 1, f_flag))
                add_mon_lite(subject_ptr, ml_ptr->mon_fy - 2, ml_ptr->mon_fx - 2, ml_ptr);
        }
    }

    s16b end_temp = tmp_pos.n;
    for (int i = 0; i < floor_ptr->mon_lite_n; i++) {
        POSITION fx = floor_ptr->mon_lite_x[i];
        POSITION fy = floor_ptr->mon_lite_y[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[fy][fx];
        if (g_ptr->info & CAVE_TEMP) {
            if ((g_ptr->info & (CAVE_VIEW | CAVE_MNLT)) == CAVE_VIEW)
                cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);
        } else if ((g_ptr->info & (CAVE_VIEW | CAVE_MNDK)) == CAVE_VIEW)
            cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);

        tmp_pos.x[tmp_pos.n] = fx;
        tmp_pos.y[tmp_pos.n] = fy;
        tmp_pos.n++;
    }

    floor_ptr->mon_lite_n = 0;
    for (int i = 0; i < end_temp; i++) {
        POSITION fx = tmp_pos.x[i];
        POSITION fy = tmp_pos.y[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[fy][fx];
        if (g_ptr->info & CAVE_MNLT) {
            if ((g_ptr->info & (CAVE_VIEW | CAVE_TEMP)) == CAVE_VIEW)
                cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);
        } else if ((g_ptr->info & (CAVE_VIEW | CAVE_XTRA)) == CAVE_VIEW)
            cave_note_and_redraw_later(floor_ptr, g_ptr, fy, fx);

        floor_ptr->mon_lite_x[floor_ptr->mon_lite_n] = fx;
        floor_ptr->mon_lite_y[floor_ptr->mon_lite_n] = fy;
        floor_ptr->mon_lite_n++;
    }

    for (int i = end_temp; i < tmp_pos.n; i++)
        floor_ptr->grid_array[tmp_pos.y[i]][tmp_pos.x[i]].info &= ~(CAVE_TEMP | CAVE_XTRA);

    tmp_pos.n = 0;
    subject_ptr->update |= PU_DELAY_VIS;
    subject_ptr->monlite = (floor_ptr->grid_array[subject_ptr->y][subject_ptr->x].info & CAVE_MNLT) != 0;
    if (!(subject_ptr->special_defense & NINJA_S_STEALTH)) {
        subject_ptr->old_monlite = subject_ptr->monlite;
        return;
    }

    if (subject_ptr->old_monlite == subject_ptr->monlite) {
        subject_ptr->old_monlite = subject_ptr->monlite;
        return;
    }

    if (subject_ptr->monlite)
        msg_print(_("影の覆いが薄れた気がする。", "Your mantle of shadow becomes thin."));
    else
        msg_print(_("影の覆いが濃くなった！", "Your mantle of shadow is restored to its original darkness."));

    subject_ptr->old_monlite = subject_ptr->monlite;
}

/*!
 * @brief 画面切り替え等でモンスターの灯りを消去する
 * @param floor_ptr 現在フロアへの参照ポインタ
 * @return なし
 */
void clear_mon_lite(floor_type *floor_ptr)
{
    for (int i = 0; i < floor_ptr->mon_lite_n; i++) {
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[floor_ptr->mon_lite_y[i]][floor_ptr->mon_lite_x[i]];
        g_ptr->info &= ~(CAVE_MNLT | CAVE_MNDK);
    }

    floor_ptr->mon_lite_n = 0;
}
