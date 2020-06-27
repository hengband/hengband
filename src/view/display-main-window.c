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
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/input-key-acceptor.h"
#include "io/targeting.h"
#include "monster/monster-update.h"
#include "player/mimic-info-table.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "view/display-map.h"
#include "view/main-window-row-column.h"
#include "view/main-window-util.h"
#include "world/world.h"

/*
 * Not using graphical tiles for this feature?
 */
#define IS_ASCII_GRAPHICS(A) (!((A)&0x80))

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
void apply_default_feat_lighting(TERM_COLOR *f_attr, SYMBOL_CODE *f_char)
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
 * Determines if a map location is currently "on screen" -RAK-
 * Note that "panel_contains(Y,X)" always implies "in_bounds2(Y,X)".
 */
bool panel_contains(POSITION y, POSITION x) { return (y >= panel_row_min) && (y <= panel_row_max) && (x >= panel_col_min) && (x <= panel_col_max); }
