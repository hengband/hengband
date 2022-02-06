#include "view/display-map.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "window/main-window-util.h"
#include "world/world.h"

byte display_autopick; /*!< 自動拾い状態の設定フラグ */

/* 一般的にオブジェクトシンボルとして扱われる記号を定義する(幻覚処理向け) /  Hack -- Legal object codes */
char image_object_hack[MAX_IMAGE_OBJECT_HACK] = "?/|\\\"!$()_-=[]{},~";

/* 一般的にモンスターシンボルとして扱われる記号を定義する(幻覚処理向け) / Hack -- Legal monster codes */
char image_monster_hack[MAX_IMAGE_MONSTER_HACK] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*!
 * @brief オブジェクトの表示を幻覚状態に差し替える / Hallucinatory object
 * @param ap 本来の色
 * @param cp 本来のシンボル
 */
static void image_object(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (use_graphics) {
        object_kind *k_ptr = &k_info[randint1(k_info.size() - 1)];
        *cp = k_ptr->x_char;
        *ap = k_ptr->x_attr;
        return;
    }

    size_t n = sizeof(image_object_hack) - 1;
    *cp = image_object_hack[randint0(n)];
    *ap = randint1(15);
}

/*!
 * @brief モンスターの表示を幻覚状態に差し替える / Mega-Hack -- Hallucinatory monster
 * @param ap 本来の色
 * @param cp 本来のシンボル
 */
static void image_monster(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (use_graphics) {
        monster_race *r_ptr = &r_info[randint1(r_info.size() - 1)];
        *cp = r_ptr->x_char;
        *ap = r_ptr->x_attr;
        return;
    }

    *cp = (one_in_(25) ? image_object_hack[randint0(sizeof(image_object_hack) - 1)] : image_monster_hack[randint0(sizeof(image_monster_hack) - 1)]);
    *ap = randint1(15);
}

/*!
 * @brief オブジェクト＆モンスターの表示を幻覚状態に差し替える / Hack -- Random hallucination
 * @param ap 本来の色
 * @param cp 本来のシンボル
 */
static void image_random(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (randint0(100) < 75) {
        image_monster(ap, cp);
    } else {
        image_object(ap, cp);
    }
}

/*!
 * @brief マップに表示されるべき地形(壁)かどうかを判定する
 * @param floor_ptr 階の情報への参照ポインタ
 * @param f_ptr 地形の情報への参照ポインタ
 * @param y グリッドy座標
 * @param x グリッドx座標
 * @return 表示されるべきならtrue、そうでないならfalse
 * @details
 * 周り全てが壁に囲まれている壁についてはオプション状態による。
 * 1か所でも空きがあるか、壁ではない地形、金を含む地形、永久岩は表示。
 */
static bool is_revealed_wall(floor_type *floor_ptr, feature_type *f_ptr, POSITION y, POSITION x)
{
    if (view_hidden_walls) {
        if (view_unsafe_walls)
            return true;
        if (none_bits(floor_ptr->grid_array[y][x].info, CAVE_UNSAFE))
            return true;
    }

    if (f_ptr->flags.has_not(FloorFeatureType::WALL) || f_ptr->flags.has(FloorFeatureType::HAS_GOLD))
        return true;

    if (in_bounds(floor_ptr, y, x) && f_ptr->flags.has(FloorFeatureType::PERMANENT))
        return true;

    int n = 0;
    for (int i = 0; i < 8; i++) {
        int dy = y + ddy_cdd[i];
        int dx = x + ddx_cdd[i];
        if (!in_bounds(floor_ptr, dy, dx)) {
            n++;
            continue;
        }

        FEAT_IDX f_idx = floor_ptr->grid_array[dy][dx].feat;
        feature_type *n_ptr = &f_info[f_idx];
        if (n_ptr->flags.has(FloorFeatureType::WALL))
            n++;
    }

    return (n != 8);
}

/*!
 * @brief 指定した座標の地形の表示属性を取得する / Extract the attr/char to display at the given (legal) map location
 * @param player_ptr プレイヤー情報への参照ポインタ
 * @param y 階の中のy座標
 * @param x 階の中のy座標
 * @param ap 文字色属性
 * @param cp 文字種属性
 * @param tap 文字色属性(タイル)
 * @param tcp 文字種属性(タイル)
 */
void map_info(PlayerType *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    FEAT_IDX feat = g_ptr->get_feat_mimic();
    feature_type *f_ptr = &f_info[feat];
    TERM_COLOR a;
    SYMBOL_CODE c;
    if (f_ptr->flags.has_not(FloorFeatureType::REMEMBER)) {
        auto is_visible = any_bits(g_ptr->info, (CAVE_MARK | CAVE_LITE | CAVE_MNLT));
        auto is_glowing = match_bits(g_ptr->info, CAVE_GLOW | CAVE_MNDK, CAVE_GLOW);
        auto can_view = g_ptr->is_view() && (is_glowing || player_ptr->see_nocto);
        if ((player_ptr->blind == 0) && (is_visible || can_view)) {
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
            if (player_ptr->wild_mode) {
                if (view_special_lite && !is_daytime()) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                }
            } else if (darkened_grid(player_ptr, g_ptr)) {
                feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;
                f_ptr = &f_info[feat];
                a = f_ptr->x_attr[F_LIT_STANDARD];
                c = f_ptr->x_char[F_LIT_STANDARD];
            } else if (view_special_lite) {
                if (g_ptr->info & (CAVE_LITE | CAVE_MNLT)) {
                    if (view_yellow_lite) {
                        a = f_ptr->x_attr[F_LIT_LITE];
                        c = f_ptr->x_char[F_LIT_LITE];
                    }
                } else if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                } else if (!(g_ptr->info & CAVE_VIEW)) {
                    if (view_bright_lite) {
                        a = f_ptr->x_attr[F_LIT_DARK];
                        c = f_ptr->x_char[F_LIT_DARK];
                    }
                }
            }
        } else {
            feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;
            f_ptr = &f_info[feat];
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
        }
    } else {
        if (g_ptr->is_mark() && is_revealed_wall(floor_ptr, f_ptr, y, x)) {
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
            if (player_ptr->wild_mode) {
                if (view_granite_lite && (player_ptr->blind || !is_daytime())) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                }
            } else if (darkened_grid(player_ptr, g_ptr) && !player_ptr->blind) {
                if (f_ptr->flags.has_all_of({ FloorFeatureType::LOS, FloorFeatureType::PROJECT })) {
                    feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;
                    f_ptr = &f_info[feat];
                    a = f_ptr->x_attr[F_LIT_STANDARD];
                    c = f_ptr->x_char[F_LIT_STANDARD];
                } else if (view_granite_lite && view_bright_lite) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                }
            } else if (view_granite_lite) {
                if (player_ptr->blind) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                } else if (g_ptr->info & (CAVE_LITE | CAVE_MNLT)) {
                    if (view_yellow_lite) {
                        a = f_ptr->x_attr[F_LIT_LITE];
                        c = f_ptr->x_char[F_LIT_LITE];
                    }
                } else if (view_bright_lite) {
                    if (!(g_ptr->info & CAVE_VIEW)) {
                        a = f_ptr->x_attr[F_LIT_DARK];
                        c = f_ptr->x_char[F_LIT_DARK];
                    } else if ((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
                        a = f_ptr->x_attr[F_LIT_DARK];
                        c = f_ptr->x_char[F_LIT_DARK];
                    } else if (f_ptr->flags.has_not(FloorFeatureType::LOS) && !check_local_illumination(player_ptr, y, x)) {
                        a = f_ptr->x_attr[F_LIT_DARK];
                        c = f_ptr->x_char[F_LIT_DARK];
                    }
                }
            }
        } else {
            feat = (view_unsafe_grids && (g_ptr->info & CAVE_UNSAFE)) ? feat_undetected : feat_none;
            f_ptr = &f_info[feat];
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
        }
    }

    if (feat_priority == -1)
        feat_priority = f_ptr->priority;

    (*tap) = a;
    (*tcp) = c;
    (*ap) = a;
    (*cp) = c;

    if (player_ptr->hallucinated && one_in_(256))
        image_random(ap, cp);

    for (const auto this_o_idx : g_ptr->o_idx_list) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        if (!(o_ptr->marked & OM_FOUND))
            continue;

        if (display_autopick) {
            byte act;

            match_autopick = find_autopick_list(player_ptr, o_ptr);
            if (match_autopick == -1)
                continue;

            act = autopick_list[match_autopick].action;

            if ((act & DO_DISPLAY) && (act & display_autopick)) {
                autopick_obj = o_ptr;
            } else {
                match_autopick = -1;
                continue;
            }
        }

        (*cp) = object_char(o_ptr);
        (*ap) = object_attr(o_ptr);
        feat_priority = 20;
        if (player_ptr->hallucinated)
            image_object(ap, cp);

        break;
    }

    if (g_ptr->m_idx && display_autopick != 0) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    monster_type *m_ptr = &floor_ptr->m_list[g_ptr->m_idx];
    if (!m_ptr->ml) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    monster_race *r_ptr = &r_info[m_ptr->ap_r_idx];
    feat_priority = 30;
    if (player_ptr->hallucinated) {
        if (r_ptr->visual_flags.has_all_of({ MonsterVisualType::CLEAR, MonsterVisualType::CLEAR_COLOR })) {
            /* Do nothing */
        } else {
            image_monster(ap, cp);
        }

        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    a = r_ptr->x_attr;
    c = r_ptr->x_char;
    if (r_ptr->visual_flags.has_none_of({ MonsterVisualType::CLEAR, MonsterVisualType::SHAPECHANGER, MonsterVisualType::CLEAR_COLOR, MonsterVisualType::MULTI_COLOR, MonsterVisualType::RANDOM_COLOR })) {
        *ap = a;
        *cp = c;
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if (r_ptr->visual_flags.has_all_of({ MonsterVisualType::CLEAR, MonsterVisualType::CLEAR_COLOR })) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if (r_ptr->visual_flags.has(MonsterVisualType::CLEAR_COLOR) && (*ap != TERM_DARK) && !use_graphics) {
        /* Do nothing */
    } else if (r_ptr->visual_flags.has(MonsterVisualType::MULTI_COLOR) && !use_graphics) {
        if (r_ptr->visual_flags.has(MonsterVisualType::ANY_COLOR))
            *ap = randint1(15);
        else
            switch (randint1(7)) {
            case 1:
                *ap = TERM_RED;
                break;
            case 2:
                *ap = TERM_L_RED;
                break;
            case 3:
                *ap = TERM_WHITE;
                break;
            case 4:
                *ap = TERM_L_GREEN;
                break;
            case 5:
                *ap = TERM_BLUE;
                break;
            case 6:
                *ap = TERM_L_DARK;
                break;
            case 7:
                *ap = TERM_GREEN;
                break;
            }
    } else if (r_ptr->visual_flags.has(MonsterVisualType::RANDOM_COLOR) && !use_graphics) {
        *ap = g_ptr->m_idx % 15 + 1;
    } else {
        *ap = a;
    }

    if (r_ptr->visual_flags.has(MonsterVisualType::CLEAR) && (*cp != ' ') && !use_graphics) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if (r_ptr->visual_flags.has(MonsterVisualType::SHAPECHANGER)) {
        if (use_graphics) {
            monster_race *tmp_r_ptr = &r_info[randint1(r_info.size() - 1)];
            *cp = tmp_r_ptr->x_char;
            *ap = tmp_r_ptr->x_attr;
        } else {
            *cp = (one_in_(25) ? image_object_hack[randint0(sizeof(image_object_hack) - 1)] : image_monster_hack[randint0(sizeof(image_monster_hack) - 1)]);
        }

        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    *cp = c;
    set_term_color(player_ptr, y, x, ap, cp);
}
