#include "target/target-setter.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "core/stuff-handler.h"
#include "floor/floor.h"
#include "game-option/cheat-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "grid/grid.h"
#include "io/cursor.h"
#include "io/input-key-requester.h"
#include "io/screen-util.h"
#include "main/sound-of-music.h"
#include "system/floor-type-definition.h"
#include "target/target-checker.h"
#include "target/target-describer.h"
#include "target/target-preparation.h"
#include "target/target-types.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "window/main-window-util.h"

/*!
 * @brief フォーカスを当てるべきマップ描画の基準座標を指定する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param y 変更先のフロアY座標
 * @param x 変更先のフロアX座標
 * @details
 * Handle a request to change the current panel
 * Return TRUE if the panel was changed.
 * Also used in do_cmd_locate
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
static bool change_panel_xy(player_type *creature_ptr, POSITION y, POSITION x)
{
    POSITION dy = 0, dx = 0;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    if (y < panel_row_min)
        dy = -1;

    if (y > panel_row_max)
        dy = 1;

    if (x < panel_col_min)
        dx = -1;

    if (x > panel_col_max)
        dx = 1;

    if (!dy && !dx)
        return FALSE;

    return change_panel(creature_ptr, dy, dx);
}

/*
 * Help "select" a location (see below)
 */
static POSITION_IDX target_pick(POSITION y1, POSITION x1, POSITION dy, POSITION dx)
{
    POSITION_IDX b_i = -1, b_v = 9999;
    for (POSITION_IDX i = 0; i < tmp_pos.n; i++) {
        POSITION x2 = tmp_pos.x[i];
        POSITION y2 = tmp_pos.y[i];
        POSITION x3 = (x2 - x1);
        POSITION y3 = (y2 - y1);
        if (dx && (x3 * dx <= 0))
            continue;

        if (dy && (y3 * dy <= 0))
            continue;

        POSITION x4 = ABS(x3);
        POSITION y4 = ABS(y3);
        if (dy && !dx && (x4 > y4))
            continue;

        if (dx && !dy && (y4 > x4))
            continue;

        POSITION_IDX v = ((x4 > y4) ? (x4 + x4 + y4) : (y4 + y4 + x4));
        if ((b_i >= 0) && (v >= b_v))
            continue;

        b_i = i;
        b_v = v;
    }

    return b_i;
}

/*
 * Handle "target" and "look".
 */
bool target_set(player_type *creature_ptr, target_type mode)
{
    POSITION y = creature_ptr->y;
    POSITION x = creature_ptr->x;
    bool done = FALSE;
    bool flag = TRUE;
    char query;
    char info[80];
    grid_type *g_ptr;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    target_who = 0;
    const char same_key = rogue_like_commands ? 'x' : 'l';
    target_set_prepare(creature_ptr, mode);
    int m = 0;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    while (!done) {
        if (flag && tmp_pos.n) {
            y = tmp_pos.y[m];
            x = tmp_pos.x[m];
            change_panel_xy(creature_ptr, y, x);
            if (!(mode & TARGET_LOOK))
                print_path(creature_ptr, y, x);

            g_ptr = &floor_ptr->grid_array[y][x];
            if (target_able(creature_ptr, g_ptr->m_idx))
                strcpy(info, _("q止 t決 p自 o現 +次 -前", "q,t,p,o,+,-,<dir>"));
            else
                strcpy(info, _("q止 p自 o現 +次 -前", "q,p,o,+,-,<dir>"));

            if (cheat_sight) {
                char cheatinfo[30];
                sprintf(cheatinfo, " LOS:%d, PROJECTABLE:%d", los(creature_ptr, creature_ptr->y, creature_ptr->x, y, x),
                    projectable(creature_ptr, creature_ptr->y, creature_ptr->x, y, x));
                strcat(info, cheatinfo);
            }

            while (TRUE) {
                query = examine_grid(creature_ptr, y, x, mode, info);
                if (query)
                    break;
            }

            int d = 0;
            if (use_menu) {
                if (query == '\r')
                    query = 't';
            }

            switch (query) {
            case ESCAPE:
            case 'q': {
                done = TRUE;
                break;
            }
            case 't':
            case '.':
            case '5':
            case '0': {
                if (!target_able(creature_ptr, g_ptr->m_idx)) {
                    bell();
                    break;
                }

                health_track(creature_ptr, g_ptr->m_idx);
                target_who = g_ptr->m_idx;
                target_row = y;
                target_col = x;
                done = TRUE;
                break;
            }
            case ' ':
            case '*':
            case '+': {
                if (++m == tmp_pos.n) {
                    m = 0;
                    if (!expand_list)
                        done = TRUE;
                }

                break;
            }
            case '-': {
                if (m-- == 0) {
                    m = tmp_pos.n - 1;
                    if (!expand_list)
                        done = TRUE;
                }

                break;
            }
            case 'p': {
                verify_panel(creature_ptr);
                creature_ptr->update |= PU_MONSTERS;
                creature_ptr->redraw |= PR_MAP;
                creature_ptr->window |= PW_OVERHEAD;
                handle_stuff(creature_ptr);
                target_set_prepare(creature_ptr, mode);
                y = creature_ptr->y;
                x = creature_ptr->x;
            }
                /* Fall through */
            case 'o':
                flag = FALSE;
                break;
            case 'm':
                break;
            default: {
                if (query == same_key) {
                    if (++m == tmp_pos.n) {
                        m = 0;
                        if (!expand_list)
                            done = TRUE;
                    }
                } else {
                    d = get_keymap_dir(query);
                    if (!d)
                        bell();

                    break;
                }
            }
            }

            if (d) {
                POSITION y2 = panel_row_min;
                POSITION x2 = panel_col_min;
                int i = target_pick(tmp_pos.y[m], tmp_pos.x[m], ddy[d], ddx[d]);
                while (flag && (i < 0)) {
                    if (change_panel(creature_ptr, ddy[d], ddx[d])) {
                        int v = tmp_pos.y[m];
                        int u = tmp_pos.x[m];
                        target_set_prepare(creature_ptr, mode);
                        flag = TRUE;
                        i = target_pick(v, u, ddy[d], ddx[d]);
                        if (i >= 0)
                            m = i;

                        continue;
                    }

                    POSITION dx = ddx[d];
                    POSITION dy = ddy[d];
                    panel_row_min = y2;
                    panel_col_min = x2;
                    panel_bounds_center();
                    creature_ptr->update |= (PU_MONSTERS);
                    creature_ptr->redraw |= (PR_MAP);
                    creature_ptr->window |= (PW_OVERHEAD);
                    handle_stuff(creature_ptr);
                    target_set_prepare(creature_ptr, mode);
                    flag = FALSE;
                    x += dx;
                    y += dy;
                    if (((x < panel_col_min + wid / 2) && (dx > 0)) || ((x > panel_col_min + wid / 2) && (dx < 0)))
                        dx = 0;

                    if (((y < panel_row_min + hgt / 2) && (dy > 0)) || ((y > panel_row_min + hgt / 2) && (dy < 0)))
                        dy = 0;

                    if ((y >= panel_row_min + hgt) || (y < panel_row_min) || (x >= panel_col_min + wid) || (x < panel_col_min)) {
                        if (change_panel(creature_ptr, dy, dx))
                            target_set_prepare(creature_ptr, mode);
                    }

                    if (x >= floor_ptr->width - 1)
                        x = floor_ptr->width - 2;
                    else if (x <= 0)
                        x = 1;

                    if (y >= floor_ptr->height - 1)
                        y = floor_ptr->height - 2;
                    else if (y <= 0)
                        y = 1;
                }

                m = i;
            }

            continue;
        }

        bool move_fast = FALSE;
        if (!(mode & TARGET_LOOK))
            print_path(creature_ptr, y, x);

        g_ptr = &floor_ptr->grid_array[y][x];
        strcpy(info, _("q止 t決 p自 m近 +次 -前", "q,t,p,m,+,-,<dir>"));
        if (cheat_sight) {
            char cheatinfo[100];
            sprintf(cheatinfo, " LOS:%d, PROJECTABLE:%d, SPECIAL:%d", los(creature_ptr, creature_ptr->y, creature_ptr->x, y, x),
                projectable(creature_ptr, creature_ptr->y, creature_ptr->x, y, x), g_ptr->special);
            strcat(info, cheatinfo);
        }

        /* Describe and Prompt (enable "TARGET_LOOK") */
        while ((query = examine_grid(creature_ptr, y, x, mode | TARGET_LOOK, info)) == 0)
            ;

        int d = 0;
        if (use_menu && (query == '\r'))
            query = 't';

        switch (query) {
        case ESCAPE:
        case 'q':
            done = TRUE;
            break;
        case 't':
        case '.':
        case '5':
        case '0':
            target_who = -1;
            target_row = y;
            target_col = x;
            done = TRUE;
            break;
        case 'p':
            verify_panel(creature_ptr);
            creature_ptr->update |= (PU_MONSTERS);
            creature_ptr->redraw |= (PR_MAP);
            creature_ptr->window |= (PW_OVERHEAD);
            handle_stuff(creature_ptr);
            target_set_prepare(creature_ptr, mode);
            y = creature_ptr->y;
            x = creature_ptr->x;
        case 'o':
            break;
        case ' ':
        case '*':
        case '+':
        case '-':
        case 'm': {
            flag = TRUE;
            m = 0;
            int bd = 999;
            for (int i = 0; i < tmp_pos.n; i++) {
                int t = distance(y, x, tmp_pos.y[i], tmp_pos.x[i]);
                if (t < bd) {
                    m = i;
                    bd = t;
                }
            }

            if (bd == 999)
                flag = FALSE;

            break;
        }
        default: {
            d = get_keymap_dir(query);
            if (isupper(query))
                move_fast = TRUE;

            if (!d)
                bell();
            break;
        }
        }

        if (d) {
            POSITION dx = ddx[d];
            POSITION dy = ddy[d];
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

            if ((y >= panel_row_min + hgt) || (y < panel_row_min) || (x >= panel_col_min + wid) || (x < panel_col_min)) {
                if (change_panel(creature_ptr, dy, dx))
                    target_set_prepare(creature_ptr, mode);
            }

            if (x >= floor_ptr->width - 1)
                x = floor_ptr->width - 2;
            else if (x <= 0)
                x = 1;

            if (y >= floor_ptr->height - 1)
                y = floor_ptr->height - 2;
            else if (y <= 0)
                y = 1;
        }
    }

    tmp_pos.n = 0;
    prt("", 0, 0);
    verify_panel(creature_ptr);
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->window |= (PW_OVERHEAD);
    handle_stuff(creature_ptr);
    return target_who != 0;
}
