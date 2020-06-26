/*!
 * @brief プレイヤーのステータス処理 / status
 * @date 2018/09/25
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "view/display-main-window.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-methods-table.h"
#include "core/player-processor.h"
#include "core/stuff-handler.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "io/input-key-acceptor.h"
#include "io/targeting.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-update.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "object/object-mark-types.h"
#include "player/mimic-info-table.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/main-window-row-column.h"
#include "view/main-window-util.h" // 相互依存している。後で何とかする.
#include "world/world.h"

/*
 * Not using graphical tiles for this feature?
 */
#define IS_ASCII_GRAPHICS(A) (!((A)&0x80))

static byte display_autopick; /*!< 自動拾い状態の設定フラグ */

/*!
 * @brief ゲーム時刻を表示する /
 * Print time
 * @return なし
 */
void print_time(player_type *player_ptr)
{
    int day, hour, min;
    c_put_str(TERM_WHITE, "             ", ROW_DAY, COL_DAY);
    extract_day_hour_min(player_ptr, &day, &hour, &min);
    if (day < 1000)
        c_put_str(TERM_WHITE, format(_("%2d日目", "Day%3d"), day), ROW_DAY, COL_DAY);
    else
        c_put_str(TERM_WHITE, _("***日目", "Day***"), ROW_DAY, COL_DAY);

    c_put_str(TERM_WHITE, format("%2d:%02d", hour, min), ROW_DAY, COL_DAY + 7);
}

/*!
 * @brief 現在のマップ名を返す /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return マップ名の文字列参照ポインタ
 */
concptr map_name(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest) && (quest[floor_ptr->inside_quest].flags & QUEST_FLAG_PRESET))
        return _("クエスト", "Quest");
    else if (creature_ptr->wild_mode)
        return _("地上", "Surface");
    else if (creature_ptr->current_floor_ptr->inside_arena)
        return _("アリーナ", "Arena");
    else if (creature_ptr->phase_out)
        return _("闘技場", "Monster Arena");
    else if (!floor_ptr->dun_level && creature_ptr->town_num)
        return town_info[creature_ptr->town_num].name;
    else
        return d_name + d_info[creature_ptr->dungeon_idx].name;
}

/*!
 * todo ここにplayer_type を追加するとz-termに影響が行くので保留
 * @brief コンソールのリサイズに合わせてマップを再描画する /
 * Map resizing whenever the main term changes size
 * @return なし
 */
void resize_map()
{
    if (!current_world_ptr->character_dungeon)
        return;

    panel_row_max = 0;
    panel_col_max = 0;
    panel_row_min = p_ptr->current_floor_ptr->height;
    panel_col_min = p_ptr->current_floor_ptr->width;
    verify_panel(p_ptr);

    p_ptr->update |= (PU_TORCH | PU_BONUS | PU_HP | PU_MANA | PU_SPELLS);
    p_ptr->update |= (PU_UN_VIEW | PU_UN_LITE);
    p_ptr->update |= (PU_VIEW | PU_LITE | PU_MON_LITE);
    p_ptr->update |= (PU_MONSTERS);
    p_ptr->redraw |= (PR_WIPE | PR_BASIC | PR_EXTRA | PR_MAP | PR_EQUIPPY);

    handle_stuff(p_ptr);
    Term_redraw();

    if (can_save)
        move_cursor_relative(p_ptr->y, p_ptr->x);

    Term_fresh();
}

/*!
 * todo ここにplayer_type を追加するとz-termに影響が行くので保留
 * @brief コンソールを再描画する /
 * Redraw a term when it is resized
 * @return なし
 */
void redraw_window(void)
{
    if (!current_world_ptr->character_dungeon)
        return;

    p_ptr->window |= (PW_INVEN | PW_EQUIP | PW_SPELL | PW_PLAYER);
    p_ptr->window |= (PW_MESSAGE | PW_OVERHEAD | PW_DUNGEON | PW_MONSTER | PW_OBJECT);

    handle_stuff(p_ptr);
    Term_redraw();
}

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する（サブルーチン）
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param dy 変更先のフロアY座標
 * @param dx 変更先のフロアX座標
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
bool change_panel(player_type *player_ptr, POSITION dy, POSITION dx)
{
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);

    POSITION y = panel_row_min + dy * hgt / 2;
    POSITION x = panel_col_min + dx * wid / 2;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (y > floor_ptr->height - hgt)
        y = floor_ptr->height - hgt;
    if (y < 0)
        y = 0;

    if (x > floor_ptr->width - wid)
        x = floor_ptr->width - wid;
    if (x < 0)
        x = 0;

    if ((y == panel_row_min) && (x == panel_col_min))
        return FALSE;

    panel_row_min = y;
    panel_col_min = x;
    panel_bounds_center();

    player_ptr->update |= (PU_MONSTERS);
    player_ptr->redraw |= (PR_MAP);
    handle_stuff(player_ptr);
    return TRUE;
}

/*!
 * @brief 現在のコンソール表示の縦横を返す。 /
 * Get term size and calculate screen size
 * @param wid_p コンソールの表示幅文字数を返す
 * @param hgt_p コンソールの表示行数を返す
 * @return なし
 */
void get_screen_size(TERM_LEN *wid_p, TERM_LEN *hgt_p)
{
    Term_get_size(wid_p, hgt_p);
    *hgt_p -= ROW_MAP + 2;
    *wid_p -= COL_MAP + 2;
    if (use_bigtile)
        *wid_p /= 2;
}

/*
 * Calculate panel colum of a location in the map
 */
int panel_col_of(int col)
{
    col -= panel_col_min;
    if (use_bigtile)
        col *= 2;
    return col + 13;
}

/* 一般的にモンスターシンボルとして扱われる記号を定義する(幻覚処理向け) / Hack -- Legal monster codes */
static char image_monster_hack[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

/* 一般的にオブジェクトシンボルとして扱われる記号を定義する(幻覚処理向け) /  Hack -- Legal object codes */
static char image_object_hack[] = "?/|\\\"!$()_-=[]{},~";

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

    int n = sizeof(image_object_hack) - 1;
    *cp = image_object_hack[randint0(n)];
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
 * 照明の表現を行うための色合いの関係を{暗闇時, 照明時} で定義する /
 * This array lists the effects of "brightness" on various "base" colours.\n
 *\n
 * This is used to do dynamic lighting effects in ascii :-)\n
 * At the moment, only the various "floor" tiles are affected.\n
 *\n
 * The layout of the array is [x][0] = light and [x][1] = dark.\n
 */
static TERM_COLOR lighting_colours[16][2] = {
    /* TERM_DARK */
    { TERM_L_DARK, TERM_DARK },

    /* TERM_WHITE */
    { TERM_YELLOW, TERM_SLATE },

    /* TERM_SLATE */
    { TERM_WHITE, TERM_L_DARK },

    /* TERM_ORANGE */
    { TERM_L_UMBER, TERM_UMBER },

    /* TERM_RED */
    { TERM_RED, TERM_RED },

    /* TERM_GREEN */
    { TERM_L_GREEN, TERM_GREEN },

    /* TERM_BLUE */
    { TERM_BLUE, TERM_BLUE },

    /* TERM_UMBER */
    { TERM_L_UMBER, TERM_RED },

    /* TERM_L_DARK */
    { TERM_SLATE, TERM_L_DARK },

    /* TERM_L_WHITE */
    { TERM_WHITE, TERM_SLATE },

    /* TERM_VIOLET */
    { TERM_L_RED, TERM_BLUE },

    /* TERM_YELLOW */
    { TERM_YELLOW, TERM_ORANGE },

    /* TERM_L_RED */
    { TERM_L_RED, TERM_L_RED },

    /* TERM_L_GREEN */
    { TERM_L_GREEN, TERM_GREEN },

    /* TERM_L_BLUE */
    { TERM_L_BLUE, TERM_L_BLUE },

    /* TERM_L_UMBER */
    { TERM_L_UMBER, TERM_UMBER }
};

/*!
 * @brief 調査中
 * @todo コメントを付加すること
 */
void apply_default_feat_lighting(TERM_COLOR f_attr[F_LIT_MAX], SYMBOL_CODE f_char[F_LIT_MAX])
{
    TERM_COLOR s_attr = f_attr[F_LIT_STANDARD];
    SYMBOL_CODE s_char = f_char[F_LIT_STANDARD];

    if (IS_ASCII_GRAPHICS(s_attr)) /* For ASCII */
    {
        f_attr[F_LIT_LITE] = lighting_colours[s_attr & 0x0f][0];
        f_attr[F_LIT_DARK] = lighting_colours[s_attr & 0x0f][1];
        for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
            f_char[i] = s_char;
    } else /* For tile graphics */
    {
        for (int i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
            f_attr[i] = s_attr;
        f_char[F_LIT_LITE] = s_char + 2;
        f_char[F_LIT_DARK] = s_char + 1;
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
    if (!have_flag(f_ptr->flags, FF_REMEMBER)) {
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
                if (have_flag(f_ptr->flags, FF_LOS) && have_flag(f_ptr->flags, FF_PROJECT)) {
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
                    } else if (!have_flag(f_ptr->flags, FF_LOS) && !check_local_illumination(player_ptr, y, x)) {
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

/*
 * Display a "small-scale" map of the dungeon for the player
 *
 * Currently, the "player" is displayed on the map.
 */
void do_cmd_view_map(player_type *player_ptr)
{
    screen_save();
    prt(_("お待ち下さい...", "Please wait..."), 0, 0);
    Term_fresh();
    Term_clear();
    display_autopick = 0;

    int cy, cx;
    display_map(player_ptr, &cy, &cx);
    if ((max_autopick == 0) || player_ptr->wild_mode) {
        put_str(_("何かキーを押すとゲームに戻ります", "Hit any key to continue"), 23, 30);
        move_cursor(cy, cx);
        inkey();
        screen_load();
        return;
    }

    display_autopick = ITEM_DISPLAY;
    while (TRUE) {
        int wid, hgt;
        Term_get_size(&wid, &hgt);
        int row_message = hgt - 1;
        put_str(_("何かキーを押してください('M':拾う 'N':放置 'D':M+N 'K':壊すアイテムを表示)",
                    " Hit M, N(for ~), K(for !), or D(same as M+N) to display auto-picker items."),
            row_message, 1);
        move_cursor(cy, cx);
        int i = inkey();
        byte flag;
        if ('M' == i)
            flag = (DO_AUTOPICK | DO_QUERY_AUTOPICK);
        else if ('N' == i)
            flag = DONT_AUTOPICK;
        else if ('K' == i)
            flag = DO_AUTODESTROY;
        else if ('D' == i)
            flag = (DO_AUTOPICK | DO_QUERY_AUTOPICK | DONT_AUTOPICK);
        else
            break;

        Term_fresh();
        if (~display_autopick & flag)
            display_autopick |= flag;
        else
            display_autopick &= ~flag;

        display_map(player_ptr, &cy, &cx);
    }

    display_autopick = 0;
    screen_load();
}

/*
 * Track a new monster
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param m_idx トラッキング対象のモンスターID。0の時キャンセル
 * @param なし
 */
void health_track(player_type *player_ptr, MONSTER_IDX m_idx)
{
    if (m_idx && m_idx == player_ptr->riding)
        return;

    player_ptr->health_who = m_idx;
    player_ptr->redraw |= (PR_HEALTH);
}

/*
 * Moves the cursor to a given MAP (y,x) location
 */
void move_cursor_relative(int row, int col)
{
    row -= panel_row_prt;
    Term_gotoxy(panel_col_of(col), row);
}

/*
 * print project path
 */
void print_path(player_type *player_ptr, POSITION y, POSITION x)
{
    int path_n;
    u16b path_g[512];
    byte default_color = TERM_SLATE;

    if (!display_path)
        return;
    if (project_length == -1)
        return;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    path_n = project_path(player_ptr, path_g, (project_length ? project_length : MAX_RANGE), player_ptr->y, player_ptr->x, y, x, PROJECT_PATH | PROJECT_THRU);
    player_ptr->redraw |= (PR_MAP);
    handle_stuff(player_ptr);
    for (int i = 0; i < path_n; i++) {
        POSITION ny = GRID_Y(path_g[i]);
        POSITION nx = GRID_X(path_g[i]);
        grid_type *g_ptr = &floor_ptr->grid_array[ny][nx];
        if (panel_contains(ny, nx)) {
            TERM_COLOR a = default_color;
            SYMBOL_CODE c;

            TERM_COLOR ta = default_color;
            SYMBOL_CODE tc = '*';

            if (g_ptr->m_idx && floor_ptr->m_list[g_ptr->m_idx].ml) {
                map_info(player_ptr, ny, nx, &a, &c, &ta, &tc);

                if (!IS_ASCII_GRAPHICS(a))
                    a = default_color;
                else if (c == '.' && (a == TERM_WHITE || a == TERM_L_WHITE))
                    a = default_color;
                else if (a == default_color)
                    a = TERM_WHITE;
            }

            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    a = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    a = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    a = TERM_L_DARK;
            }

            c = '*';
            Term_queue_bigchar(panel_col_of(nx), ny - panel_row_prt, a, c, ta, tc);
        }

        if ((g_ptr->info & CAVE_MARK) && !cave_have_flag_grid(g_ptr, FF_PROJECT))
            break;

        if (nx == x && ny == y)
            default_color = TERM_L_DARK;
    }
}

/*
 * Track the given monster race
 */
void monster_race_track(player_type *player_ptr, MONRACE_IDX r_idx)
{
    player_ptr->monster_race_idx = r_idx;
    player_ptr->window |= (PW_MONSTER);
}

/*
 * Track the given object kind
 */
void object_kind_track(player_type *player_ptr, KIND_OBJECT_IDX k_idx)
{
    player_ptr->object_kind_idx = k_idx;
    player_ptr->window |= (PW_OBJECT);
}

/*!
 * @brief 実ゲームプレイ時間を更新する
 */
void update_playtime(void)
{
    if (current_world_ptr->start_time != 0) {
        u32b tmp = (u32b)time(NULL);
        current_world_ptr->play_time += (tmp - current_world_ptr->start_time);
        current_world_ptr->start_time = tmp;
    }
}

bool panel_contains(POSITION y, POSITION x) { return (y >= panel_row_min) && (y <= panel_row_max) && (x >= panel_col_min) && (x <= panel_col_max); }

/*
 * Delayed visual update
 * Only used if update_view(), update_lite() or update_mon_lite() was called
 */
void delayed_visual_update(player_type *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < floor_ptr->redraw_n; i++) {
        POSITION y = floor_ptr->redraw_y[i];
        POSITION x = floor_ptr->redraw_x[i];
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];
        if (!(g_ptr->info & CAVE_REDRAW))
            continue;

        if (g_ptr->info & CAVE_NOTE)
            note_spot(player_ptr, y, x);

        lite_spot(player_ptr, y, x);
        if (g_ptr->m_idx)
            update_monster(player_ptr, g_ptr->m_idx, FALSE);

        g_ptr->info &= ~(CAVE_NOTE | CAVE_REDRAW);
    }

    floor_ptr->redraw_n = 0;
}
