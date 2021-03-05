﻿/*!
 * @brief 地形に関する情報を表示する
 * @date 2020/04/24
 * @author Hourier
 */

#include "knowledge/knowledge-features.h"
#include "core/show-file.h"
#include "dungeon/dungeon.h"
#include "game-option/special-options.h"
#include "io-dump/dump-util.h"
#include "io/input-key-acceptor.h"
#include "knowledge/lighting-level-table.h"
#include "monster-race/monster-race.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "world/world.h"

/*
 * Build a list of feature indexes in the given group. Return the number
 * of features in the group.
 *
 * mode & 0x01 : check for non-empty group
 */
static FEAT_IDX collect_features(FEAT_IDX *feat_idx, BIT_FLAGS8 mode)
{
    FEAT_IDX feat_cnt = 0;
    for (FEAT_IDX i = 0; i < max_f_idx; i++) {
        feature_type *f_ptr = &f_info[i];
        if (!f_ptr->name)
            continue;
        if (f_ptr->mimic != i)
            continue;

        feat_idx[feat_cnt++] = i;
        if (mode & 0x01)
            break;
    }

    feat_idx[feat_cnt] = -1;
    return feat_cnt;
}

/*
 * Display the features in a group.
 */
static void display_feature_list(int col, int row, int per_page, FEAT_IDX *feat_idx, FEAT_IDX feat_cur, FEAT_IDX feat_top, bool visual_only, int lighting_level)
{
    int lit_col[F_LIT_MAX], i;
    int f_idx_col = use_bigtile ? 62 : 64;

    lit_col[F_LIT_STANDARD] = use_bigtile ? (71 - F_LIT_MAX) : 71;
    for (i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++)
        lit_col[i] = lit_col[F_LIT_STANDARD] + 2 + (i - F_LIT_NS_BEGIN) * 2 + (use_bigtile ? i : 0);

    for (i = 0; i < per_page && (feat_idx[feat_top + i] >= 0); i++) {
        TERM_COLOR attr;
        FEAT_IDX f_idx = feat_idx[feat_top + i];
        feature_type *f_ptr = &f_info[f_idx];
        int row_i = row + i;
        attr = ((i + feat_top == feat_cur) ? TERM_L_BLUE : TERM_WHITE);
        c_prt(attr, f_name + f_ptr->name, row_i, col);
        if (per_page == 1) {
            c_prt(attr, format("(%s)", lighting_level_str[lighting_level]), row_i, col + 1 + strlen(f_name + f_ptr->name));
            c_prt(attr, format("%02x/%02x", f_ptr->x_attr[lighting_level], f_ptr->x_char[lighting_level]), row_i,
                f_idx_col - ((current_world_ptr->wizard || visual_only) ? 6 : 2));
        }
        if (current_world_ptr->wizard || visual_only) {
            c_prt(attr, format("%d", f_idx), row_i, f_idx_col);
        }

        term_queue_bigchar(lit_col[F_LIT_STANDARD], row_i, f_ptr->x_attr[F_LIT_STANDARD], f_ptr->x_char[F_LIT_STANDARD], 0, 0);
        term_putch(lit_col[F_LIT_NS_BEGIN], row_i, TERM_SLATE, '(');
        for (int j = F_LIT_NS_BEGIN + 1; j < F_LIT_MAX; j++) {
            term_putch(lit_col[j], row_i, TERM_SLATE, '/');
        }

        term_putch(lit_col[F_LIT_MAX - 1] + (use_bigtile ? 3 : 2), row_i, TERM_SLATE, ')');
        for (int j = F_LIT_NS_BEGIN; j < F_LIT_MAX; j++) {
            term_queue_bigchar(lit_col[j] + 1, row_i, f_ptr->x_attr[j], f_ptr->x_char[j], 0, 0);
        }
    }

    for (; i < per_page; i++) {
        term_erase(col, row + i, 255);
    }
}

/*
 * Interact with feature visuals.
 */
void do_cmd_knowledge_features(bool *need_redraw, bool visual_only, IDX direct_f_idx, IDX *lighting_level)
{
    TERM_COLOR attr_old[F_LIT_MAX];
    (void)C_WIPE(attr_old, F_LIT_MAX, TERM_COLOR);
    SYMBOL_CODE char_old[F_LIT_MAX];
    (void)C_WIPE(char_old, F_LIT_MAX, SYMBOL_CODE);

    TERM_LEN wid, hgt;
    term_get_size(&wid, &hgt);

    FEAT_IDX *feat_idx;
    C_MAKE(feat_idx, max_f_idx, FEAT_IDX);

    concptr feature_group_text[] = { "terrains", NULL };
    int len;
    int max = 0;
    int grp_cnt = 0;
    int feat_cnt;
    FEAT_IDX grp_idx[100];
    TERM_COLOR attr_top = 0;
    bool visual_list = FALSE;
    byte char_left = 0;
    TERM_LEN browser_rows = hgt - 8;
    if (direct_f_idx < 0) {
        for (FEAT_IDX i = 0; feature_group_text[i] != NULL; i++) {
            len = strlen(feature_group_text[i]);
            if (len > max)
                max = len;

            if (collect_features(feat_idx, 0x01)) {
                grp_idx[grp_cnt++] = i;
            }
        }

        feat_cnt = 0;
    } else {
        feature_type *f_ptr = &f_info[direct_f_idx];

        feat_idx[0] = direct_f_idx;
        feat_cnt = 1;
        feat_idx[1] = -1;

        (void)visual_mode_command('v', &visual_list, browser_rows - 1, wid - (max + 3), &attr_top, &char_left, &f_ptr->x_attr[*lighting_level],
            &f_ptr->x_char[*lighting_level], need_redraw);

        for (FEAT_IDX i = 0; i < F_LIT_MAX; i++) {
            attr_old[i] = f_ptr->x_attr[i];
            char_old[i] = f_ptr->x_char[i];
        }
    }

    grp_idx[grp_cnt] = -1;

    FEAT_IDX old_grp_cur = -1;
    FEAT_IDX grp_cur = 0;
    FEAT_IDX grp_top = 0;
    FEAT_IDX feat_cur = 0;
    FEAT_IDX feat_top = 0;
    TERM_LEN column = 0;
    bool flag = FALSE;
    bool redraw = TRUE;
    TERM_COLOR *cur_attr_ptr;
    SYMBOL_CODE *cur_char_ptr;
    while (!flag) {
        char ch;
        feature_type *f_ptr;

        if (redraw) {
            clear_from(0);

            prt(_("表示 - 地形", "Visuals - features"), 2, 0);
            if (direct_f_idx < 0)
                prt(_("グループ", "Group"), 4, 0);
            prt(_("名前", "Name"), 4, max + 3);
            if (use_bigtile) {
                if (current_world_ptr->wizard || visual_only)
                    prt("Idx", 4, 62);
                prt(_("文字 ( l/ d)", "Sym ( l/ d)"), 4, 66);
            } else {
                if (current_world_ptr->wizard || visual_only)
                    prt("Idx", 4, 64);
                prt(_("文字 (l/d)", "Sym (l/d)"), 4, 68);
            }

            for (FEAT_IDX i = 0; i < 78; i++) {
                term_putch(i, 5, TERM_WHITE, '=');
            }

            if (direct_f_idx < 0) {
                for (FEAT_IDX i = 0; i < browser_rows; i++) {
                    term_putch(max + 1, 6 + i, TERM_WHITE, '|');
                }
            }

            redraw = FALSE;
        }

        if (direct_f_idx < 0) {
            if (grp_cur < grp_top)
                grp_top = grp_cur;
            if (grp_cur >= grp_top + browser_rows)
                grp_top = grp_cur - browser_rows + 1;

            display_group_list(0, 6, max, browser_rows, grp_idx, feature_group_text, grp_cur, grp_top);
            if (old_grp_cur != grp_cur) {
                old_grp_cur = grp_cur;
                feat_cnt = collect_features(feat_idx, 0x00);
            }

            while (feat_cur < feat_top)
                feat_top = MAX(0, feat_top - browser_rows / 2);
            while (feat_cur >= feat_top + browser_rows)
                feat_top = MIN(feat_cnt - browser_rows, feat_top + browser_rows / 2);
        }

        if (!visual_list) {
            display_feature_list(max + 3, 6, browser_rows, feat_idx, feat_cur, feat_top, visual_only, F_LIT_STANDARD);
        } else {
            feat_top = feat_cur;
            display_feature_list(max + 3, 6, 1, feat_idx, feat_cur, feat_top, visual_only, *lighting_level);
            display_visual_list(max + 3, 7, browser_rows - 1, wid - (max + 3), attr_top, char_left);
        }

        prt(format(_("<方向>%s, 'd'で標準光源効果%s, ESC", "<dir>%s, 'd' for default lighting%s, ESC"),
                visual_list ? _(", ENTERで決定, 'a'で対象明度変更", ", ENTER to accept, 'a' for lighting level")
                            : _(", 'v'でシンボル変更", ", 'v' for visuals"),
                (attr_idx || char_idx) ? _(", 'c', 'p'でペースト", ", 'c', 'p' to paste") : _(", 'c'でコピー", ", 'c' to copy")),
            hgt - 1, 0);

        f_ptr = &f_info[feat_idx[feat_cur]];
        cur_attr_ptr = &f_ptr->x_attr[*lighting_level];
        cur_char_ptr = &f_ptr->x_char[*lighting_level];

        if (visual_list) {
            place_visual_list_cursor(max + 3, 7, *cur_attr_ptr, *cur_char_ptr, attr_top, char_left);
        } else if (!column) {
            term_gotoxy(0, 6 + (grp_cur - grp_top));
        } else {
            term_gotoxy(max + 3, 6 + (feat_cur - feat_top));
        }

        ch = inkey();
        if (visual_list && ((ch == 'A') || (ch == 'a'))) {
            int prev_lighting_level = *lighting_level;

            if (ch == 'A') {
                if (*lighting_level <= 0)
                    *lighting_level = F_LIT_MAX - 1;
                else
                    (*lighting_level)--;
            } else {
                if (*lighting_level >= F_LIT_MAX - 1)
                    *lighting_level = 0;
                else
                    (*lighting_level)++;
            }

            if (f_ptr->x_attr[prev_lighting_level] != f_ptr->x_attr[*lighting_level])
                attr_top = MAX(0, (f_ptr->x_attr[*lighting_level] & 0x7f) - 5);

            if (f_ptr->x_char[prev_lighting_level] != f_ptr->x_char[*lighting_level])
                char_left = MAX(0, f_ptr->x_char[*lighting_level] - 10);

            continue;
        } else if ((ch == 'D') || (ch == 'd')) {
            TERM_COLOR prev_x_attr = f_ptr->x_attr[*lighting_level];
            byte prev_x_char = f_ptr->x_char[*lighting_level];

            apply_default_feat_lighting(f_ptr->x_attr, f_ptr->x_char);

            if (visual_list) {
                if (prev_x_attr != f_ptr->x_attr[*lighting_level])
                    attr_top = MAX(0, (f_ptr->x_attr[*lighting_level] & 0x7f) - 5);

                if (prev_x_char != f_ptr->x_char[*lighting_level])
                    char_left = MAX(0, f_ptr->x_char[*lighting_level] - 10);
            } else
                *need_redraw = TRUE;

            continue;
        } else if (visual_mode_command(ch, &visual_list, browser_rows - 1, wid - (max + 3), &attr_top, &char_left, cur_attr_ptr, cur_char_ptr, need_redraw)) {
            switch (ch) {
            case ESCAPE:
                for (FEAT_IDX i = 0; i < F_LIT_MAX; i++) {
                    f_ptr->x_attr[i] = attr_old[i];
                    f_ptr->x_char[i] = char_old[i];
                }

                /* Fall through */
            case '\n':
            case '\r':
                if (direct_f_idx >= 0)
                    flag = TRUE;
                else
                    *lighting_level = F_LIT_STANDARD;
                break;
            case 'V':
            case 'v':
                for (FEAT_IDX i = 0; i < F_LIT_MAX; i++) {
                    attr_old[i] = f_ptr->x_attr[i];
                    char_old[i] = f_ptr->x_char[i];
                }
                *lighting_level = F_LIT_STANDARD;
                break;

            case 'C':
            case 'c':
                if (!visual_list) {
                    for (FEAT_IDX i = 0; i < F_LIT_MAX; i++) {
                        attr_idx_feat[i] = f_ptr->x_attr[i];
                        char_idx_feat[i] = f_ptr->x_char[i];
                    }
                }
                break;

            case 'P':
            case 'p':
                if (!visual_list) {
                    for (FEAT_IDX i = F_LIT_NS_BEGIN; i < F_LIT_MAX; i++) {
                        if (attr_idx_feat[i] || (!(char_idx_feat[i] & 0x80) && char_idx_feat[i]))
                            f_ptr->x_attr[i] = attr_idx_feat[i];
                        if (char_idx_feat[i])
                            f_ptr->x_char[i] = char_idx_feat[i];
                    }
                }
                break;
            }
            continue;
        }

        switch (ch) {
        case ESCAPE: {
            flag = TRUE;
            break;
        }

        default: {
            browser_cursor(ch, &column, &grp_cur, grp_cnt, &feat_cur, feat_cnt);
            break;
        }
        }
    }

    C_KILL(feat_idx, max_f_idx, FEAT_IDX);
}

/*
 * Dungeon
 */
void do_cmd_knowledge_dungeon(player_type *creature_ptr)
{
    FILE *fff = NULL;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name))
        return;

    for (int i = 1; i < current_world_ptr->max_d_idx; i++) {
        bool seiha = FALSE;

        if (!d_info[i].maxdepth)
            continue;
        if (!max_dlv[i])
            continue;
        if (d_info[i].final_guardian) {
            if (!r_info[d_info[i].final_guardian].max_num)
                seiha = TRUE;
        } else if (max_dlv[i] == d_info[i].maxdepth)
            seiha = TRUE;

        fprintf(fff, _("%c%-12s :  %3d 階\n", "%c%-16s :  level %3d\n"), seiha ? '!' : ' ', d_name + d_info[i].name, (int)max_dlv[i]);
    }

    angband_fclose(fff);
    (void)show_file(creature_ptr, TRUE, file_name, _("今までに入ったダンジョン", "Dungeon"), 0, 0);
    fd_kill(file_name);
}
