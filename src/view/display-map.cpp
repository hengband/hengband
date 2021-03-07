#include "view/display-map.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-methods-table.h"
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
 * @return なし
 */
static void image_object(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (use_graphics) {
        object_kind *k_ptr = &k_info[randint1(max_k_idx - 1)];
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
 * @return なし
 */
static void image_monster(TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (use_graphics) {
        monster_race *r_ptr = &r_info[randint1(max_r_idx - 1)];
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
 * @return なし
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
 * @brief Mコマンドによる縮小マップの表示を行う / Extract the attr/char to display at the given (legal) map location
 */
void map_info(player_type *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp, TERM_COLOR *tap, SYMBOL_CODE *tcp)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    OBJECT_IDX this_o_idx, next_o_idx = 0;
    FEAT_IDX feat = get_feat_mimic(g_ptr);
    feature_type *f_ptr = &f_info[feat];
    TERM_COLOR a;
    SYMBOL_CODE c;
    if (!has_flag(f_ptr->flags, FF_REMEMBER)) {
        if (!player_ptr->blind
            && ((g_ptr->info & (CAVE_MARK | CAVE_LITE | CAVE_MNLT))
                || ((g_ptr->info & CAVE_VIEW) && (((g_ptr->info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW) || player_ptr->see_nocto)))) {
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
        if (g_ptr->info & CAVE_MARK) {
            a = f_ptr->x_attr[F_LIT_STANDARD];
            c = f_ptr->x_char[F_LIT_STANDARD];
            if (player_ptr->wild_mode) {
                if (view_granite_lite && (player_ptr->blind || !is_daytime())) {
                    a = f_ptr->x_attr[F_LIT_DARK];
                    c = f_ptr->x_char[F_LIT_DARK];
                }
            } else if (darkened_grid(player_ptr, g_ptr) && !player_ptr->blind) {
                if (has_flag(f_ptr->flags, FF_LOS) && has_flag(f_ptr->flags, FF_PROJECT)) {
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
                    } else if (!has_flag(f_ptr->flags, FF_LOS) && !check_local_illumination(player_ptr, y, x)) {
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

    if (player_ptr->image && one_in_(256))
        image_random(ap, cp);

    for (this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
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
        if (player_ptr->image)
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
    if (player_ptr->image) {
        if ((r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) == (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) {
            /* Do nothing */
        } else {
            image_monster(ap, cp);
        }

        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    a = r_ptr->x_attr;
    c = r_ptr->x_char;
    if (!(r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_SHAPECHANGER | RF1_ATTR_CLEAR | RF1_ATTR_MULTI | RF1_ATTR_SEMIRAND))) {
        *ap = a;
        *cp = c;
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if ((r_ptr->flags1 & (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) == (RF1_CHAR_CLEAR | RF1_ATTR_CLEAR)) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if ((r_ptr->flags1 & RF1_ATTR_CLEAR) && (*ap != TERM_DARK) && !use_graphics) {
        /* Do nothing */
    } else if ((r_ptr->flags1 & RF1_ATTR_MULTI) && !use_graphics) {
        if (r_ptr->flags2 & RF2_ATTR_ANY)
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
    } else if ((r_ptr->flags1 & RF1_ATTR_SEMIRAND) && !use_graphics) {
        *ap = g_ptr->m_idx % 15 + 1;
    } else {
        *ap = a;
    }

    if ((r_ptr->flags1 & RF1_CHAR_CLEAR) && (*cp != ' ') && !use_graphics) {
        set_term_color(player_ptr, y, x, ap, cp);
        return;
    }

    if (r_ptr->flags1 & RF1_SHAPECHANGER) {
        if (use_graphics) {
            monster_race *tmp_r_ptr = &r_info[randint1(max_r_idx - 1)];
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
