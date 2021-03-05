﻿#include "window/main-window-util.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/geometry.h"
#include "game-option/map-screen-options.h"
#include "game-option/special-options.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-map.h"
#include "world/world.h"

/*
 * Dungeon size info
 */
POSITION panel_row_min;
POSITION panel_row_max;
POSITION panel_col_min;
POSITION panel_col_max;
POSITION panel_col_prt;
POSITION panel_row_prt;

int match_autopick;
object_type *autopick_obj; /*!< 各種自動拾い処理時に使うオブジェクトポインタ */
int feat_priority; /*!< マップ縮小表示時に表示すべき地形の優先度を保管する */

static concptr simplify_list[][2] = {
#ifdef JP
    { "の魔法書", "" }, { NULL, NULL }
#else
    { "^Ring of ", "=" }, { "^Amulet of ", "\"" }, { "^Scroll of ", "?" }, { "^Scroll titled ", "?" }, { "^Wand of ", "-" }, { "^Rod of ", "-" },
    { "^Staff of ", "_" }, { "^Potion of ", "!" }, { " Spellbook ", "" }, { "^Book of ", "" }, { " Magic [", "[" }, { " Book [", "[" }, { " Arts [", "[" },
    { "^Set of ", "" }, { "^Pair of ", "" }, { NULL, NULL }
#endif
};

/*!
 * @brief 画面左の能力値表示を行うために指定位置から13キャラ分を空白消去後指定のメッセージを明るい青で描画する /
 * Print character info at given row, column in a 13 char field
 * @param info 表示文字列
 * @param row 描画列
 * @param col 描画行
 * @return なし
 */
void print_field(concptr info, TERM_LEN row, TERM_LEN col)
{
    c_put_str(TERM_WHITE, "             ", row, col);
    c_put_str(TERM_L_BLUE, info, row, col);
}

/*
 * Prints the map of the dungeon
 *
 * Note that, for efficiency, we contain an "optimized" version
 * of both "lite_spot()" and "print_rel()", and that we use the
 * "lite_spot()" function to display the player grid, if needed.
 */
void print_map(player_type *player_ptr)
{
    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);

    wid -= COL_MAP + 2;
    hgt -= ROW_MAP + 2;

    int v;
    (void)term_get_cursor(&v);

    (void)term_set_cursor(0);

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    POSITION xmin = (0 < panel_col_min) ? panel_col_min : 0;
    POSITION xmax = (floor_ptr->width - 1 > panel_col_max) ? panel_col_max : floor_ptr->width - 1;
    POSITION ymin = (0 < panel_row_min) ? panel_row_min : 0;
    POSITION ymax = (floor_ptr->height - 1 > panel_row_max) ? panel_row_max : floor_ptr->height - 1;

    for (POSITION y = 1; y <= ymin - panel_row_prt; y++) {
        term_erase(COL_MAP, y, wid);
    }

    for (POSITION y = ymax - panel_row_prt; y <= hgt; y++) {
        term_erase(COL_MAP, y, wid);
    }

    for (POSITION y = ymin; y <= ymax; y++) {
        for (POSITION x = xmin; x <= xmax; x++) {
            TERM_COLOR a;
            SYMBOL_CODE c;
            TERM_COLOR ta;
            SYMBOL_CODE tc;
            map_info(player_ptr, y, x, &a, &c, &ta, &tc);
            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    a = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    a = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    a = TERM_L_DARK;
            }

            term_queue_bigchar(panel_col_of(x), y - panel_row_prt, a, c, ta, tc);
        }
    }

    lite_spot(player_ptr, player_ptr->y, player_ptr->x);
    (void)term_set_cursor(v);
}

static void display_shortened_item_name(player_type *player_ptr, object_type *o_ptr, int y)
{
    char buf[MAX_NLEN];
    describe_flavor(player_ptr, buf, o_ptr, (OD_NO_FLAVOR | OD_OMIT_PREFIX | OD_NAME_ONLY));
    TERM_COLOR attr = tval_to_attr[o_ptr->tval % 128];

    if (player_ptr->image) {
        attr = TERM_WHITE;
        strcpy(buf, _("何か奇妙な物", "something strange"));
    }

    char *c = buf;
    for (c = buf; *c; c++) {
        for (int i = 0; simplify_list[i][1]; i++) {
            concptr org_w = simplify_list[i][0];

            if (*org_w == '^') {
                if (c == buf)
                    org_w++;
                else
                    continue;
            }

            if (strncmp(c, org_w, strlen(org_w)))
                continue;

            char *s = c;
            concptr tmp = simplify_list[i][1];
            while (*tmp)
                *s++ = *tmp++;
            tmp = c + strlen(org_w);
            while (*tmp)
                *s++ = *tmp++;
            *s = '\0';
        }
    }

    c = buf;
    int len = 0;
    /* 半角 12 文字分で切る */
    while (*c) {
#ifdef JP
        if (iskanji(*c)) {
            if (len + 2 > 12)
                break;
            c += 2;
            len += 2;
        } else
#endif
        {
            if (len + 1 > 12)
                break;
            c++;
            len++;
        }
    }

    *c = '\0';
    term_putstr(0, y, 12, attr, buf);
}

/*
 * Display a "small-scale" map of the dungeon in the active Term
 */
void display_map(player_type *player_ptr, int *cy, int *cx)
{
    int i, j, x, y;

    TERM_COLOR ta;
    SYMBOL_CODE tc;

    byte tp;

    TERM_COLOR **bigma;
    SYMBOL_CODE **bigmc;
    byte **bigmp;

    TERM_COLOR **ma;
    SYMBOL_CODE **mc;
    byte **mp;

    bool old_view_special_lite = view_special_lite;
    bool old_view_granite_lite = view_granite_lite;
    TERM_LEN hgt, wid, yrat, xrat;
    int **match_autopick_yx;
    object_type ***object_autopick_yx;
    term_get_size(&wid, &hgt);
    hgt -= 2;
    wid -= 14;
    if (use_bigtile)
        wid /= 2;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    yrat = (floor_ptr->height + hgt - 1) / hgt;
    xrat = (floor_ptr->width + wid - 1) / wid;
    view_special_lite = FALSE;
    view_granite_lite = FALSE;

    C_MAKE(ma, (hgt + 2), TERM_COLOR *);
    C_MAKE(mc, (hgt + 2), char_ptr);
    C_MAKE(mp, (hgt + 2), byte_ptr);
    C_MAKE(match_autopick_yx, (hgt + 2), int *);
    C_MAKE(object_autopick_yx, (hgt + 2), object_type **);
    for (y = 0; y < (hgt + 2); y++) {
        C_MAKE(ma[y], (wid + 2), TERM_COLOR);
        C_MAKE(mc[y], (wid + 2), char);
        C_MAKE(mp[y], (wid + 2), byte);
        C_MAKE(match_autopick_yx[y], (wid + 2), int);
        C_MAKE(object_autopick_yx[y], (wid + 2), object_type *);
        for (x = 0; x < wid + 2; ++x) {
            match_autopick_yx[y][x] = -1;
            object_autopick_yx[y][x] = NULL;
            ma[y][x] = TERM_WHITE;
            mc[y][x] = ' ';
            mp[y][x] = 0;
        }
    }

    C_MAKE(bigma, (floor_ptr->height + 2), TERM_COLOR *);
    C_MAKE(bigmc, (floor_ptr->height + 2), char_ptr);
    C_MAKE(bigmp, (floor_ptr->height + 2), byte_ptr);
    for (y = 0; y < (floor_ptr->height + 2); y++) {
        C_MAKE(bigma[y], (floor_ptr->width + 2), TERM_COLOR);
        C_MAKE(bigmc[y], (floor_ptr->width + 2), char);
        C_MAKE(bigmp[y], (floor_ptr->width + 2), byte);
        for (x = 0; x < floor_ptr->width + 2; ++x) {
            bigma[y][x] = TERM_WHITE;
            bigmc[y][x] = ' ';
            bigmp[y][x] = 0;
        }
    }

    for (i = 0; i < floor_ptr->width; ++i) {
        for (j = 0; j < floor_ptr->height; ++j) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            match_autopick = -1;
            autopick_obj = NULL;
            feat_priority = -1;
            map_info(player_ptr, j, i, &ta, &tc, &ta, &tc);
            tp = (byte)feat_priority;
            if (match_autopick != -1 && (match_autopick_yx[y][x] == -1 || match_autopick_yx[y][x] > match_autopick)) {
                match_autopick_yx[y][x] = match_autopick;
                object_autopick_yx[y][x] = autopick_obj;
                tp = 0x7f;
            }

            bigmc[j + 1][i + 1] = tc;
            bigma[j + 1][i + 1] = ta;
            bigmp[j + 1][i + 1] = tp;
        }
    }

    for (j = 0; j < floor_ptr->height; ++j) {
        for (i = 0; i < floor_ptr->width; ++i) {
            x = i / xrat + 1;
            y = j / yrat + 1;

            tc = bigmc[j + 1][i + 1];
            ta = bigma[j + 1][i + 1];
            tp = bigmp[j + 1][i + 1];
            if (mp[y][x] == tp) {
                int t;
                int cnt = 0;

                for (t = 0; t < 8; t++) {
                    if (tc == bigmc[j + 1 + ddy_cdd[t]][i + 1 + ddx_cdd[t]] && ta == bigma[j + 1 + ddy_cdd[t]][i + 1 + ddx_cdd[t]])
                        cnt++;
                }
                if (cnt <= 4)
                    tp++;
            }

            if (mp[y][x] < tp) {
                mc[y][x] = tc;
                ma[y][x] = ta;
                mp[y][x] = tp;
            }
        }
    }

    x = wid + 1;
    y = hgt + 1;

    mc[0][0] = mc[0][x] = mc[y][0] = mc[y][x] = '+';
    for (x = 1; x <= wid; x++)
        mc[0][x] = mc[y][x] = '-';

    for (y = 1; y <= hgt; y++)
        mc[y][0] = mc[y][x] = '|';

    for (y = 0; y < hgt + 2; ++y) {
        term_gotoxy(COL_MAP, y);
        for (x = 0; x < wid + 2; ++x) {
            ta = ma[y][x];
            tc = mc[y][x];
            if (!use_graphics) {
                if (current_world_ptr->timewalk_m_idx)
                    ta = TERM_DARK;
                else if (is_invuln(player_ptr) || player_ptr->timewalk)
                    ta = TERM_WHITE;
                else if (player_ptr->wraith_form)
                    ta = TERM_L_DARK;
            }

            term_add_bigch(ta, tc);
        }
    }

    for (y = 1; y < hgt + 1; ++y) {
        match_autopick = -1;
        for (x = 1; x <= wid; x++) {
            if (match_autopick_yx[y][x] != -1 && (match_autopick > match_autopick_yx[y][x] || match_autopick == -1)) {
                match_autopick = match_autopick_yx[y][x];
                autopick_obj = object_autopick_yx[y][x];
            }
        }

        term_putstr(0, y, 12, 0, "            ");
        if (match_autopick != -1)
            display_shortened_item_name(player_ptr, autopick_obj, y);
    }

    (*cy) = player_ptr->y / yrat + 1 + ROW_MAP;
    if (!use_bigtile)
        (*cx) = player_ptr->x / xrat + 1 + COL_MAP;
    else
        (*cx) = (player_ptr->x / xrat + 1) * 2 + COL_MAP;

    view_special_lite = old_view_special_lite;
    view_granite_lite = old_view_granite_lite;

    for (y = 0; y < (hgt + 2); y++) {
        C_KILL(ma[y], (wid + 2), TERM_COLOR);
        C_KILL(mc[y], (wid + 2), SYMBOL_CODE);
        C_KILL(mp[y], (wid + 2), byte);
        C_KILL(match_autopick_yx[y], (wid + 2), int);
        C_KILL(object_autopick_yx[y], (wid + 2), object_type *);
    }

    C_KILL(ma, (hgt + 2), TERM_COLOR *);
    C_KILL(mc, (hgt + 2), char_ptr);
    C_KILL(mp, (hgt + 2), byte_ptr);
    C_KILL(match_autopick_yx, (hgt + 2), int *);
    C_KILL(object_autopick_yx, (hgt + 2), object_type **);
    for (y = 0; y < (floor_ptr->height + 2); y++) {
        C_KILL(bigma[y], (floor_ptr->width + 2), TERM_COLOR);
        C_KILL(bigmc[y], (floor_ptr->width + 2), SYMBOL_CODE);
        C_KILL(bigmp[y], (floor_ptr->width + 2), byte);
    }

    C_KILL(bigma, (floor_ptr->height + 2), TERM_COLOR *);
    C_KILL(bigmc, (floor_ptr->height + 2), char_ptr);
    C_KILL(bigmp, (floor_ptr->height + 2), byte_ptr);
}

void set_term_color(player_type *player_ptr, POSITION y, POSITION x, TERM_COLOR *ap, SYMBOL_CODE *cp)
{
    if (!player_bold(player_ptr, y, x))
        return;

    monster_race *r_ptr = &r_info[0];
    *ap = r_ptr->x_attr;
    *cp = r_ptr->x_char;
    feat_priority = 31;
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
