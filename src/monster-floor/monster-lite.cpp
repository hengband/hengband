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
#include "player-base/player-class.h"
#include "player-info/ninja-data-type.h"
#include "player/special-defense-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/point-2d.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <vector>

/*!
 * @brief モンスターによる光量状態更新 / Add a square to the changes array
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param points 座標たちを記録する配列
 * @param y Y座標
 * @param x X座標
 */
static void update_monster_lite(
    PlayerType *const player_ptr, std::vector<Pos2D> &points, const POSITION y, const POSITION x, const monster_lite_type *const ml_ptr)
{
    grid_type *g_ptr;
    int dpf, d;
    POSITION midpoint;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    if ((g_ptr->info & (CAVE_MNLT | CAVE_VIEW)) != CAVE_VIEW) {
        return;
    }

    if (!feat_supports_los(g_ptr->feat)) {
        if (((y < player_ptr->y) && (y > ml_ptr->mon_fy)) || ((y > player_ptr->y) && (y < ml_ptr->mon_fy))) {
            dpf = player_ptr->y - ml_ptr->mon_fy;
            d = y - ml_ptr->mon_fy;
            midpoint = ml_ptr->mon_fx + ((player_ptr->x - ml_ptr->mon_fx) * std::abs(d)) / std::abs(dpf);
            if (x < midpoint) {
                if (!cave_los_bold(player_ptr->current_floor_ptr, y, x + 1)) {
                    return;
                }
            } else if (x > midpoint) {
                if (!cave_los_bold(player_ptr->current_floor_ptr, y, x - 1)) {
                    return;
                }
            } else if (ml_ptr->mon_invis) {
                return;
            }
        }

        if (((x < player_ptr->x) && (x > ml_ptr->mon_fx)) || ((x > player_ptr->x) && (x < ml_ptr->mon_fx))) {
            dpf = player_ptr->x - ml_ptr->mon_fx;
            d = x - ml_ptr->mon_fx;
            midpoint = ml_ptr->mon_fy + ((player_ptr->y - ml_ptr->mon_fy) * std::abs(d)) / std::abs(dpf);
            if (y < midpoint) {
                if (!cave_los_bold(player_ptr->current_floor_ptr, y + 1, x)) {
                    return;
                }
            } else if (y > midpoint) {
                if (!cave_los_bold(player_ptr->current_floor_ptr, y - 1, x)) {
                    return;
                }
            } else if (ml_ptr->mon_invis) {
                return;
            }
        }
    }

    if (!(g_ptr->info & CAVE_MNDK)) {
        points.emplace_back(y, x);
    } else {
        g_ptr->info &= ~(CAVE_MNDK);
    }

    g_ptr->info |= CAVE_MNLT;
}

/*
 * Add a square to the changes array
 */
static void update_monster_dark(
    PlayerType *const player_ptr, std::vector<Pos2D> &points, const POSITION y, const POSITION x, const monster_lite_type *const ml_ptr)
{
    grid_type *g_ptr;
    int midpoint, dpf, d;
    g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
    if ((g_ptr->info & (CAVE_LITE | CAVE_MNLT | CAVE_MNDK | CAVE_VIEW)) != CAVE_VIEW) {
        return;
    }

    if (!feat_supports_los(g_ptr->feat) && !g_ptr->cave_has_flag(FloorFeatureType::PROJECT)) {
        if (((y < player_ptr->y) && (y > ml_ptr->mon_fy)) || ((y > player_ptr->y) && (y < ml_ptr->mon_fy))) {
            dpf = player_ptr->y - ml_ptr->mon_fy;
            d = y - ml_ptr->mon_fy;
            midpoint = ml_ptr->mon_fx + ((player_ptr->x - ml_ptr->mon_fx) * std::abs(d)) / std::abs(dpf);
            if (x < midpoint) {
                if (!cave_los_bold(player_ptr->current_floor_ptr, y, x + 1) && !cave_has_flag_bold(player_ptr->current_floor_ptr, y, x + 1, FloorFeatureType::PROJECT)) {
                    return;
                }
            } else if (x > midpoint) {
                if (!cave_los_bold(player_ptr->current_floor_ptr, y, x - 1) && !cave_has_flag_bold(player_ptr->current_floor_ptr, y, x - 1, FloorFeatureType::PROJECT)) {
                    return;
                }
            } else if (ml_ptr->mon_invis) {
                return;
            }
        }

        if (((x < player_ptr->x) && (x > ml_ptr->mon_fx)) || ((x > player_ptr->x) && (x < ml_ptr->mon_fx))) {
            dpf = player_ptr->x - ml_ptr->mon_fx;
            d = x - ml_ptr->mon_fx;
            midpoint = ml_ptr->mon_fy + ((player_ptr->y - ml_ptr->mon_fy) * std::abs(d)) / std::abs(dpf);
            if (y < midpoint) {
                if (!cave_los_bold(player_ptr->current_floor_ptr, y + 1, x) && !cave_has_flag_bold(player_ptr->current_floor_ptr, y + 1, x, FloorFeatureType::PROJECT)) {
                    return;
                }
            } else if (y > midpoint) {
                if (!cave_los_bold(player_ptr->current_floor_ptr, y - 1, x) && !cave_has_flag_bold(player_ptr->current_floor_ptr, y - 1, x, FloorFeatureType::PROJECT)) {
                    return;
                }
            } else if (ml_ptr->mon_invis) {
                return;
            }
        }
    }

    points.emplace_back(y, x);
    g_ptr->info |= CAVE_MNDK;
}

/*!
 * @brief Update squares illuminated or darkened by monsters.
 * The CAVE_TEMP and CAVE_XTRA flag are used to store the state during the
 * updating.  Only squares in view of the player, whos state
 * changes are drawn via lite_spot().
 * @todo player-status からのみ呼ばれている。しかしあちらは行数が酷いので要調整
 */
void update_mon_lite(PlayerType *player_ptr)
{
    // 座標たちを記録する配列。
    std::vector<Pos2D> points;

    void (*add_mon_lite)(PlayerType *, std::vector<Pos2D> &, const POSITION, const POSITION, const monster_lite_type *);
    int dis_lim = (dungeons_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::DARKNESS) && !player_ptr->see_nocto) ? (MAX_SIGHT / 2 + 1) : (MAX_SIGHT + 3);
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < floor_ptr->mon_lite_n; i++) {
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[floor_ptr->mon_lite_y[i]][floor_ptr->mon_lite_x[i]];
        g_ptr->info |= (g_ptr->info & CAVE_MNLT) ? CAVE_TEMP : CAVE_XTRA;
        g_ptr->info &= ~(CAVE_MNLT | CAVE_MNDK);
    }

    if (!w_ptr->timewalk_m_idx) {
        monster_type *m_ptr;
        monster_race *r_ptr;
        for (int i = 1; i < floor_ptr->m_max; i++) {
            m_ptr = &floor_ptr->m_list[i];
            r_ptr = &monraces_info[m_ptr->r_idx];
            if (!m_ptr->is_valid() || (m_ptr->cdis > dis_lim)) {
                continue;
            }

            int rad = 0;
            if (r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_SELF_LITE_1)) {
                rad++;
            }

            if (r_ptr->flags7 & (RF7_HAS_LITE_2 | RF7_SELF_LITE_2)) {
                rad += 2;
            }

            if (r_ptr->flags7 & (RF7_HAS_DARK_1 | RF7_SELF_DARK_1)) {
                rad--;
            }

            if (r_ptr->flags7 & (RF7_HAS_DARK_2 | RF7_SELF_DARK_2)) {
                rad -= 2;
            }

            if (!rad) {
                continue;
            }

            FloorFeatureType f_flag;
            if (rad > 0) {
                if (!(r_ptr->flags7 & (RF7_SELF_LITE_1 | RF7_SELF_LITE_2)) && (m_ptr->is_asleep() || (!floor_ptr->dun_level && is_daytime()) || player_ptr->phase_out)) {
                    continue;
                }

                if (dungeons_info[player_ptr->dungeon_idx].flags.has(DungeonFeatureType::DARKNESS)) {
                    rad = 1;
                }

                add_mon_lite = update_monster_lite;
                f_flag = FloorFeatureType::LOS;
            } else {
                if (!(r_ptr->flags7 & (RF7_SELF_DARK_1 | RF7_SELF_DARK_2)) && (m_ptr->is_asleep() || (!floor_ptr->dun_level && !is_daytime()))) {
                    continue;
                }

                add_mon_lite = update_monster_dark;
                f_flag = FloorFeatureType::PROJECT;
                rad = -rad;
            }

            monster_lite_type tmp_ml;
            monster_lite_type *ml_ptr = initialize_monster_lite_type(floor_ptr->grid_array[m_ptr->fy][m_ptr->fx].info, &tmp_ml, m_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy, ml_ptr->mon_fx, ml_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 1, ml_ptr->mon_fx, ml_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 1, ml_ptr->mon_fx, ml_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy, ml_ptr->mon_fx + 1, ml_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy, ml_ptr->mon_fx - 1, ml_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 1, ml_ptr->mon_fx + 1, ml_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 1, ml_ptr->mon_fx - 1, ml_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 1, ml_ptr->mon_fx + 1, ml_ptr);
            add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 1, ml_ptr->mon_fx - 1, ml_ptr);
            if (rad < 2) {
                continue;
            }

            grid_type *g_ptr;
            if (cave_has_flag_bold(player_ptr->current_floor_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx, f_flag)) {
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 2, ml_ptr->mon_fx + 1, ml_ptr);
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 2, ml_ptr->mon_fx, ml_ptr);
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 2, ml_ptr->mon_fx - 1, ml_ptr);
                g_ptr = &floor_ptr->grid_array[ml_ptr->mon_fy + 2][ml_ptr->mon_fx];
                if ((rad == 3) && g_ptr->cave_has_flag(f_flag)) {
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 3, ml_ptr->mon_fx + 1, ml_ptr);
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 3, ml_ptr->mon_fx, ml_ptr);
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 3, ml_ptr->mon_fx - 1, ml_ptr);
                }
            }

            if (cave_has_flag_bold(player_ptr->current_floor_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx, f_flag)) {
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 2, ml_ptr->mon_fx + 1, ml_ptr);
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 2, ml_ptr->mon_fx, ml_ptr);
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 2, ml_ptr->mon_fx - 1, ml_ptr);
                g_ptr = &floor_ptr->grid_array[ml_ptr->mon_fy - 2][ml_ptr->mon_fx];
                if ((rad == 3) && g_ptr->cave_has_flag(f_flag)) {
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 3, ml_ptr->mon_fx + 1, ml_ptr);
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 3, ml_ptr->mon_fx, ml_ptr);
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 3, ml_ptr->mon_fx - 1, ml_ptr);
                }
            }

            if (cave_has_flag_bold(player_ptr->current_floor_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx + 1, f_flag)) {
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 1, ml_ptr->mon_fx + 2, ml_ptr);
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy, ml_ptr->mon_fx + 2, ml_ptr);
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 1, ml_ptr->mon_fx + 2, ml_ptr);
                g_ptr = &floor_ptr->grid_array[ml_ptr->mon_fy][ml_ptr->mon_fx + 2];
                if ((rad == 3) && g_ptr->cave_has_flag(f_flag)) {
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 1, ml_ptr->mon_fx + 3, ml_ptr);
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy, ml_ptr->mon_fx + 3, ml_ptr);
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 1, ml_ptr->mon_fx + 3, ml_ptr);
                }
            }

            if (cave_has_flag_bold(player_ptr->current_floor_ptr, ml_ptr->mon_fy, ml_ptr->mon_fx - 1, f_flag)) {
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 1, ml_ptr->mon_fx - 2, ml_ptr);
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy, ml_ptr->mon_fx - 2, ml_ptr);
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 1, ml_ptr->mon_fx - 2, ml_ptr);
                g_ptr = &floor_ptr->grid_array[ml_ptr->mon_fy][ml_ptr->mon_fx - 2];
                if ((rad == 3) && g_ptr->cave_has_flag(f_flag)) {
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 1, ml_ptr->mon_fx - 3, ml_ptr);
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy, ml_ptr->mon_fx - 3, ml_ptr);
                    add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 1, ml_ptr->mon_fx - 3, ml_ptr);
                }
            }

            if (rad != 3) {
                continue;
            }

            if (cave_has_flag_bold(player_ptr->current_floor_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx + 1, f_flag)) {
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 2, ml_ptr->mon_fx + 2, ml_ptr);
            }

            if (cave_has_flag_bold(player_ptr->current_floor_ptr, ml_ptr->mon_fy + 1, ml_ptr->mon_fx - 1, f_flag)) {
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy + 2, ml_ptr->mon_fx - 2, ml_ptr);
            }

            if (cave_has_flag_bold(player_ptr->current_floor_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx + 1, f_flag)) {
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 2, ml_ptr->mon_fx + 2, ml_ptr);
            }

            if (cave_has_flag_bold(player_ptr->current_floor_ptr, ml_ptr->mon_fy - 1, ml_ptr->mon_fx - 1, f_flag)) {
                add_mon_lite(player_ptr, points, ml_ptr->mon_fy - 2, ml_ptr->mon_fx - 2, ml_ptr);
            }
        }
    }

    const auto end_temp = size(points);
    for (int i = 0; i < floor_ptr->mon_lite_n; i++) {
        POSITION fx = floor_ptr->mon_lite_x[i];
        POSITION fy = floor_ptr->mon_lite_y[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[fy][fx];
        if (g_ptr->info & CAVE_TEMP) {
            if ((g_ptr->info & (CAVE_VIEW | CAVE_MNLT)) == CAVE_VIEW) {
                cave_note_and_redraw_later(floor_ptr, fy, fx);
            }
        } else if ((g_ptr->info & (CAVE_VIEW | CAVE_MNDK)) == CAVE_VIEW) {
            cave_note_and_redraw_later(floor_ptr, fy, fx);
        }

        points.emplace_back(fy, fx);
    }

    floor_ptr->mon_lite_n = 0;
    for (size_t i = 0; i < end_temp; i++) {
        const auto [fy, fx] = points[i];

        grid_type *const g_ptr = &floor_ptr->grid_array[fy][fx];
        if (g_ptr->info & CAVE_MNLT) {
            if ((g_ptr->info & (CAVE_VIEW | CAVE_TEMP)) == CAVE_VIEW) {
                cave_note_and_redraw_later(floor_ptr, fy, fx);
            }
        } else if ((g_ptr->info & (CAVE_VIEW | CAVE_XTRA)) == CAVE_VIEW) {
            cave_note_and_redraw_later(floor_ptr, fy, fx);
        }

        floor_ptr->mon_lite_x[floor_ptr->mon_lite_n] = fx;
        floor_ptr->mon_lite_y[floor_ptr->mon_lite_n] = fy;
        floor_ptr->mon_lite_n++;
    }

    for (size_t i = end_temp; i < size(points); i++) {
        const auto [y, x] = points[i];
        floor_ptr->grid_array[y][x].info &= ~(CAVE_TEMP | CAVE_XTRA);
    }

    player_ptr->update |= PU_DELAY_VIS;
    player_ptr->monlite = (floor_ptr->grid_array[player_ptr->y][player_ptr->x].info & CAVE_MNLT) != 0;
    auto ninja_data = PlayerClass(player_ptr).get_specific_data<ninja_data_type>();
    if (!ninja_data || !ninja_data->s_stealth) {
        player_ptr->old_monlite = player_ptr->monlite;
        return;
    }

    if (player_ptr->old_monlite == player_ptr->monlite) {
        player_ptr->old_monlite = player_ptr->monlite;
        return;
    }

    if (player_ptr->monlite) {
        msg_print(_("影の覆いが薄れた気がする。", "Your mantle of shadow becomes thin."));
    } else {
        msg_print(_("影の覆いが濃くなった！", "Your mantle of shadow is restored to its original darkness."));
    }

    player_ptr->old_monlite = player_ptr->monlite;
}

/*!
 * @brief 画面切り替え等でモンスターの灯りを消去する
 * @param floor_ptr 現在フロアへの参照ポインタ
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
