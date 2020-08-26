#include "target/grid-selector.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "core/stuff-handler.h"
#include "floor/cave.h"
#include "game-option/game-play-options.h"
#include "game-option/keymap-directory-getter.h"
#include "grid/feature-flag-types.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/screen-util.h"
#include "system/floor-type-definition.h"
#include "target/target-checker.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"

/*
 * XAngband: determine if a given location is "interesting"
 * based on target_set_accept function.
 */
static bool tgt_pt_accept(player_type *creature_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (!(in_bounds(floor_ptr, y, x)))
        return FALSE;

    if ((y == creature_ptr->y) && (x == creature_ptr->x))
        return TRUE;

    if (creature_ptr->image)
        return FALSE;

    grid_type *g_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (!(g_ptr->info & (CAVE_MARK)))
        return FALSE;

    if (cave_has_flag_grid(g_ptr, FF_LESS) || cave_has_flag_grid(g_ptr, FF_MORE) || cave_has_flag_grid(g_ptr, FF_QUEST_ENTER)
        || cave_has_flag_grid(g_ptr, FF_QUEST_EXIT))
        return TRUE;

    return FALSE;
}

/*
 * XAngband: Prepare the "temp" array for "tget_pt"
 * based on target_set_prepare funciton.
 */
static void tgt_pt_prepare(player_type *creature_ptr)
{
    tmp_pos.n = 0;
    if (!expand_list)
        return;

    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (POSITION y = 1; y < floor_ptr->height; y++) {
        for (POSITION x = 1; x < floor_ptr->width; x++) {
            if (!tgt_pt_accept(creature_ptr, y, x))
                continue;

            tmp_pos.x[tmp_pos.n] = x;
            tmp_pos.y[tmp_pos.n] = y;
            tmp_pos.n++;
        }
    }

    ang_sort(creature_ptr, tmp_pos.x, tmp_pos.y, tmp_pos.n, ang_sort_comp_distance, ang_sort_swap_distance);
}

/*
 * old -- from PsiAngband.
 */
bool tgt_pt(player_type *creature_ptr, POSITION *x_ptr, POSITION *y_ptr)
{
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);

    POSITION x = creature_ptr->x;
    POSITION y = creature_ptr->y;
    if (expand_list)
        tgt_pt_prepare(creature_ptr);

    msg_print(_("場所を選んでスペースキーを押して下さい。", "Select a point and press space."));
    msg_flag = FALSE;

    char ch = 0;
    int n = 0;
    bool success = FALSE;
    while ((ch != ESCAPE) && !success) {
        bool move_fast = FALSE;
        move_cursor_relative(y, x);
        ch = inkey();
        switch (ch) {
        case ESCAPE:
            break;
        case ' ':
        case 't':
        case '.':
        case '5':
        case '0':
            if (player_bold(creature_ptr, y, x))
                ch = 0;
            else
                success = TRUE;

            break;
        case '>':
        case '<': {
            if (!expand_list || !tmp_pos.n)
                break;

            int dx, dy;
            int cx = (panel_col_min + panel_col_max) / 2;
            int cy = (panel_row_min + panel_row_max) / 2;
            n++;
            for (; n < tmp_pos.n; ++n) {
                grid_type *g_ptr = &creature_ptr->current_floor_ptr->grid_array[tmp_pos.y[n]][tmp_pos.x[n]];
                if (cave_has_flag_grid(g_ptr, FF_STAIRS) && cave_has_flag_grid(g_ptr, ch == '>' ? FF_MORE : FF_LESS))
                    break;
            }

            if (n == tmp_pos.n) {
                n = 0;
                y = creature_ptr->y;
                x = creature_ptr->x;
                verify_panel(creature_ptr);
                creature_ptr->update |= PU_MONSTERS;
                creature_ptr->redraw |= PR_MAP;
                creature_ptr->window |= PW_OVERHEAD;
                handle_stuff(creature_ptr);
            } else {
                y = tmp_pos.y[n];
                x = tmp_pos.x[n];
                dy = 2 * (y - cy) / hgt;
                dx = 2 * (x - cx) / wid;
                if (dy || dx)
                    change_panel(creature_ptr, dy, dx);
            }

            break;
        }
        default: {
            int d = get_keymap_dir(ch);
            if (isupper(ch))
                move_fast = TRUE;

            if (d == 0)
                break;

            int dx = ddx[d];
            int dy = ddy[d];
            if (move_fast) {
                int mag = MIN(wid / 2, hgt / 2);
                x += dx * mag;
                y += dy * mag;
            } else {
                x += dx;
                y += dy;
            }

            if (((x < panel_col_min + wid / 2) && (dx > 0)) || ((x > panel_col_min + wid / 2) && (dx < 0)))
                dx = 0;

            if (((y < panel_row_min + hgt / 2) && (dy > 0)) || ((y > panel_row_min + hgt / 2) && (dy < 0)))
                dy = 0;

            if ((y >= panel_row_min + hgt) || (y < panel_row_min) || (x >= panel_col_min + wid) || (x < panel_col_min))
                change_panel(creature_ptr, dy, dx);

            if (x >= creature_ptr->current_floor_ptr->width - 1)
                x = creature_ptr->current_floor_ptr->width - 2;
            else if (x <= 0)
                x = 1;

            if (y >= creature_ptr->current_floor_ptr->height - 1)
                y = creature_ptr->current_floor_ptr->height - 2;
            else if (y <= 0)
                y = 1;

            break;
        }
        }
    }

    prt("", 0, 0);
    verify_panel(creature_ptr);
    creature_ptr->update |= PU_MONSTERS;
    creature_ptr->redraw |= PR_MAP;
    creature_ptr->window |= PW_OVERHEAD;
    handle_stuff(creature_ptr);
    *x_ptr = x;
    *y_ptr = y;
    return success;
}
