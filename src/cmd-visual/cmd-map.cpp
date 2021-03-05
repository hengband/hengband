﻿#include "cmd-visual/cmd-map.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "io/input-key-acceptor.h"
#include "term/screen-processor.h"
#include "view/display-map.h"
#include "window/main-window-util.h"

/*
 * Display a "small-scale" map of the dungeon for the player
 *
 * Currently, the "player" is displayed on the map.
 */
void do_cmd_view_map(player_type *player_ptr)
{
    screen_save();
    prt(_("お待ち下さい...", "Please wait..."), 0, 0);
    term_fresh();
    term_clear();
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
        term_get_size(&wid, &hgt);
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

        term_fresh();
        if (~display_autopick & flag)
            display_autopick |= flag;
        else
            display_autopick &= ~flag;

        display_map(player_ptr, &cy, &cx);
    }

    display_autopick = 0;
    screen_load();
}
