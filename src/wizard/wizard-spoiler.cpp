﻿/*!
 * @brief スポイラー出力処理 (行数の都合でモンスター進化ツリーもここに入っている)
 * @date 2014/02/17
 * @author
 * Copyright (c) 1997 Ben Harrison, and others
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2013 Deskull rearranged comment for Doxygen.
 * 2020 Hourier rearranged for decreasing lines.
 */

#include "wizard/wizard-spoiler.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags8.h"
#include "system/angband-version.h"
#include "term/screen-processor.h"
#include "util/angband-files.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "wizard/fixed-artifacts-spoiler.h"
#include "wizard/items-spoiler.h"
#include "wizard/monster-info-spoiler.h"
#include "wizard/spoiler-util.h"

/*!
 * @brief int配列でstrncmp()と似た比較処理を行う /
 * Compare two int-type array like strncmp() and return TRUE if equals
 * @param a 比較するint配列1
 * @param b 比較するint配列2
 * @param length
 * @return 両者の値が等しければTRUEを返す
 */
static bool int_n_cmp(int *a, int *b, int length)
{
    if (!length)
        return TRUE;

    do {
        if (*a != *(b++))
            return FALSE;
        if (!(*(a++)))
            break;
    } while (--length);

    return TRUE;
}

/*!
 * @brief ある木が指定された木の部分木かどうかを返す /
 * Returns TRUE if an evolution tree is "partial tree"
 * @param tree 元となる木構造リスト
 * @param partial_tree 部分木かどうか判定したい木構造リスト
 * @return 部分木ならばTRUEを返す
 */
static bool is_partial_tree(int *tree, int *partial_tree)
{
    int pt_head = *(partial_tree++);
    int pt_len = 0;
    while (partial_tree[pt_len])
        pt_len++;

    while (*tree) {
        if (*(tree++) == pt_head) {
            if (int_n_cmp(tree, partial_tree, pt_len))
                return TRUE;
        }
    }

    return FALSE;
}

/*!
 * @brief 進化ツリーをスポイラー出力するメインルーチン /
 * Print monsters' evolution information to file
 * @param fname 出力ファイル名
 * @return なし
 */
static spoiler_output_status spoil_mon_evol(concptr fname)
{
    char buf[1024];
    monster_race *r_ptr;
    player_type dummy;
    int **evol_tree, i, j, n, r_idx;
    int *evol_tree_zero; /* For C_KILL() */
    path_build(buf, sizeof buf, ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return SPOILER_OUTPUT_FAIL_FOPEN;
    }

    char title[200];
    put_version(title);
    sprintf(buf, "Monster Spoilers for %s\n", title);
    spoil_out(buf);

    spoil_out("------------------------------------------\n\n");
    C_MAKE(evol_tree, max_r_idx, int *);
    C_MAKE(*evol_tree, max_r_idx * (max_evolution_depth + 1), int);
    for (i = 1; i < max_r_idx; i++)
        evol_tree[i] = *evol_tree + i * (max_evolution_depth + 1);

    evol_tree_zero = *evol_tree;
    for (i = 1; i < max_r_idx; i++) {
        r_ptr = &r_info[i];
        if (!r_ptr->next_exp)
            continue;

        n = 0;
        evol_tree[i][n++] = i;
        do {
            evol_tree[i][n++] = r_ptr->next_r_idx;
            r_ptr = &r_info[r_ptr->next_r_idx];
        } while (r_ptr->next_exp && (n < max_evolution_depth));
    }

    for (i = 1; i < max_r_idx; i++) {
        if (!evol_tree[i][0])
            continue;

        for (j = 1; j < max_r_idx; j++) {
            if (i == j)
                continue;

            if (!evol_tree[j][0])
                continue;

            if (is_partial_tree(evol_tree[j], evol_tree[i])) {
                evol_tree[i][0] = 0;
                break;
            }
        }
    }

    ang_sort(&dummy, evol_tree, NULL, max_r_idx, ang_sort_comp_evol_tree, ang_sort_swap_evol_tree);
    for (i = 0; i < max_r_idx; i++) {
        r_idx = evol_tree[i][0];
        if (!r_idx)
            continue;

        r_ptr = &r_info[r_idx];
        fprintf(spoiler_file, _("[%d]: %s (レベル%d, '%c')\n", "[%d]: %s (Level %d, '%c')\n"), r_idx, r_name + r_ptr->name, (int)r_ptr->level, r_ptr->d_char);
        for (n = 1; r_ptr->next_exp; n++) {
            fprintf(spoiler_file, "%*s-(%ld)-> ", n * 2, "", (long int)r_ptr->next_exp);
            fprintf(spoiler_file, "[%d]: ", r_ptr->next_r_idx);
            r_ptr = &r_info[r_ptr->next_r_idx];
            fprintf(spoiler_file, _("%s (レベル%d, '%c')\n", "%s (Level %d, '%c')\n"), r_name + r_ptr->name, (int)r_ptr->level, r_ptr->d_char);
        }

        fputc('\n', spoiler_file);
    }

    C_KILL(evol_tree_zero, max_r_idx * (max_evolution_depth + 1), int);
    C_KILL(evol_tree, max_r_idx, int *);
    if (ferror(spoiler_file) || angband_fclose(spoiler_file)) {
        return SPOILER_OUTPUT_FAIL_FCLOSE;
    }
    return SPOILER_OUTPUT_SUCCESS;
}

/*!
 * @brief スポイラー出力を行うコマンドのメインルーチン /
 * Create Spoiler files -BEN-
 * @return なし
 */
void exe_output_spoilers(void)
{
    screen_save();
    while (TRUE) {
        spoiler_output_status status = SPOILER_OUTPUT_CANCEL;
        term_clear();
        prt("Create a spoiler file.", 2, 0);
        prt("(1) Brief Object Info (obj-desc.txt)", 5, 5);
        prt("(2) Brief Artifact Info (artifact.txt)", 6, 5);
        prt("(3) Brief Monster Info (mon-desc.txt)", 7, 5);
        prt("(4) Full Monster Info (mon-info.txt)", 8, 5);
        prt("(5) Monster Evolution Info (mon-evol.txt)", 9, 5);
        prt(_("コマンド:", "Command: "), _(18, 12), 0);
        switch (inkey()) {
        case ESCAPE:
            screen_load();
            return;
        case '1':
            status = spoil_obj_desc("obj-desc.txt");
            break;
        case '2':
            status = spoil_fixed_artifact("artifact.txt");
            break;
        case '3':
            status = spoil_mon_desc_all("mon-desc.txt");
            break;
        case '4':
            status = spoil_mon_info("mon-info.txt");
            break;
        case '5':
            status = spoil_mon_evol("mon-evol.txt");
            break;
        default:
            bell();
            break;
        }

        switch (status) {
        case SPOILER_OUTPUT_FAIL_FOPEN:
            msg_print("Cannot create spoiler file.");
            break;
        case SPOILER_OUTPUT_FAIL_FCLOSE:
            msg_print("Cannot close spoiler file.");
            break;
        case SPOILER_OUTPUT_SUCCESS:
            msg_print("Successfully created a spoiler file.");
            break;
        }
        msg_erase();
    }
}

/*!
 * @brief 全スポイラー出力を行うコマンドのメインルーチン /
 * Create Spoiler files -BEN-
 * @return 成功時SPOILER_OUTPUT_SUCCESS / 失敗時エラー状態
 */
spoiler_output_status output_all_spoilers(void)
{
    spoiler_output_status status;
    status = spoil_obj_desc("obj-desc.txt");
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_fixed_artifact("artifact.txt");
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_mon_desc_all("mon-desc.txt");
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_mon_desc("mon-desc-wildonly.txt", FALSE, RF8_WILD_ONLY);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-town.txt", FALSE, RF8_WILD_TOWN);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-shore.txt", FALSE, RF8_WILD_SHORE);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-ocean.txt", FALSE, RF8_WILD_OCEAN);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-waste.txt", FALSE, RF8_WILD_WASTE);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-wood.txt", FALSE, RF8_WILD_WOOD);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-volcano.txt", FALSE, RF8_WILD_VOLCANO);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-mountain.txt", FALSE, RF8_WILD_MOUNTAIN);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-grass.txt", FALSE, RF8_WILD_GRASS);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;
    status = spoil_mon_desc("mon-desc-wildall.txt", FALSE, RF8_WILD_ALL);
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_mon_info("mon-info.txt");
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;

    status = spoil_mon_evol("mon-evol.txt");
    if (status != SPOILER_OUTPUT_SUCCESS)
        return status;

    return SPOILER_OUTPUT_SUCCESS;
}
