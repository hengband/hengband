/*!
 * @brief ウィザードモードの処理(スポイラー出力中心) / Spoiler generation -BEN-
 * @date 2014/02/17
 * @author
 * Copyright (c) 1997 Ben Harrison, and others
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "wizard/wizard-spoiler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-town.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags7.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-enchant/trg-types.h"
#include "object/object-flags.h"
#include "object/object-generator.h"
#include "object/object-info.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "store/store-util.h"
#include "store/store.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/quarks.h"
#include "util/sort.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "wizard/artifact-analyzer.h"
#include "wizard/fixed-artifact-spoiler.h"
#include "wizard/items-spoiler.h"
#include "wizard/monster-spoiler.h"
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
static void spoil_mon_evol(player_type *player_ptr, concptr fname)
{
    char buf[1024];
    monster_race *r_ptr;
    int **evol_tree, i, j, n, r_idx;
    int *evol_tree_zero; /* For C_KILL() */
    path_build(buf, sizeof buf, ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        msg_print("Cannot create spoiler file.");
        return;
    }

    sprintf(buf, "Monster Spoilers for Hengband Version %d.%d.%d\n", FAKE_VER_MAJOR - 10, FAKE_VER_MINOR, FAKE_VER_PATCH);
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

    ang_sort(player_ptr, evol_tree, NULL, max_r_idx, ang_sort_comp_evol_tree, ang_sort_swap_evol_tree);
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
        msg_print("Cannot close spoiler file.");
        return;
    }

    msg_print("Successfully created a spoiler file.");
}

/*!
 * @brief スポイラー出力を行うコマンドのメインルーチン /
 * Create Spoiler files -BEN-
 * @return なし
 */
void exe_output_spoilers(player_type *player_ptr)
{
    screen_save();
    while (TRUE) {
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
            spoil_obj_desc(player_ptr, "obj-desc.txt");
            break;
        case '2':
            spoil_fixed_artifact(player_ptr, "artifact.txt");
            break;
        case '3':
            spoil_mon_desc(player_ptr, "mon-desc.txt");
            break;
        case '4':
            spoil_mon_info(player_ptr, "mon-info.txt");
            break;
        case '5':
            spoil_mon_evol(player_ptr, "mon-evol.txt");
            break;
        default:
            bell();
            break;
        }

        msg_erase();
    }
}

/*!
 * @brief ランダムアーティファクト1件をスポイラー出力する /
 * Create a spoiler file entry for an artifact
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param art_ptr 記述内容を収めた構造体参照ポインタ
 * Fill in an object description structure for a given object
 * @return なし
 */
static void spoiler_print_randart(object_type *o_ptr, obj_desc_list *art_ptr)
{
    pval_info_type *pval_ptr = &art_ptr->pval_info;
    char buf[80];
    fprintf(spoiler_file, "%s\n", art_ptr->description);
    if (!object_is_fully_known(o_ptr)) {
        fprintf(spoiler_file, _("%s不明\n", "%sUnknown\n"), spoiler_indent);
    } else {
        if (pval_ptr->pval_desc[0]) {
            sprintf(buf, _("%sの修正:", "%s to"), pval_ptr->pval_desc);
            spoiler_outlist(buf, pval_ptr->pval_affects, item_separator);
        }

        spoiler_outlist(_("対:", "Slay"), art_ptr->slays, item_separator);
        spoiler_outlist(_("武器属性:", ""), art_ptr->brands, list_separator);
        spoiler_outlist(_("免疫:", "Immunity to"), art_ptr->immunities, item_separator);
        spoiler_outlist(_("耐性:", "Resist"), art_ptr->resistances, item_separator);
        spoiler_outlist(_("維持:", "Sustain"), art_ptr->sustains, item_separator);
        spoiler_outlist("", art_ptr->misc_magic, list_separator);
        if (art_ptr->activation) {
            fprintf(spoiler_file, _("%s発動: %s\n", "%sActivates for %s\n"), spoiler_indent, art_ptr->activation);
        }
    }

    fprintf(spoiler_file, "%s%s\n\n", spoiler_indent, art_ptr->misc_desc);
}

/*!
 * @brief ランダムアーティファクト内容をスポイラー出力するサブルーチン /
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param i 出力したい記録ランダムアーティファクトID
 * @return なし
 */
static void spoil_random_artifact_aux(player_type *player_ptr, object_type *o_ptr, int i)
{
    obj_desc_list artifact;
    if (!object_is_known(o_ptr) || !o_ptr->art_name || o_ptr->tval != group_artifact[i].tval)
        return;

    random_artifact_analyze(player_ptr, o_ptr, &artifact);
    spoiler_print_randart(o_ptr, &artifact);
}

/*!
 * @brief ランダムアーティファクト内容をスポイラー出力するメインルーチン /
 * Create a list file for random artifacts
 * @param fname 出力ファイル名
 * @return なし
 */
void spoil_random_artifact(player_type *creature_ptr, concptr fname)
{
    int i, j;
    store_type *store_ptr;
    object_type *q_ptr;
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        msg_print("Cannot create list file.");
        return;
    }

    sprintf(buf, "Random artifacts list.\r");
    spoiler_underline(buf);
    for (j = 0; group_artifact[j].tval; j++) {
        for (i = INVEN_RARM; i < INVEN_TOTAL; i++) {
            q_ptr = &creature_ptr->inventory_list[i];
            spoil_random_artifact_aux(creature_ptr, q_ptr, j);
        }

        for (i = 0; i < INVEN_PACK; i++) {
            q_ptr = &creature_ptr->inventory_list[i];
            spoil_random_artifact_aux(creature_ptr, q_ptr, j);
        }

        store_ptr = &town_info[1].store[STORE_HOME];
        for (i = 0; i < store_ptr->stock_num; i++) {
            q_ptr = &store_ptr->stock[i];
            spoil_random_artifact_aux(creature_ptr, q_ptr, j);
        }

        store_ptr = &town_info[1].store[STORE_MUSEUM];
        for (i = 0; i < store_ptr->stock_num; i++) {
            q_ptr = &store_ptr->stock[i];
            spoil_random_artifact_aux(creature_ptr, q_ptr, j);
        }
    }

    if (ferror(spoiler_file) || angband_fclose(spoiler_file)) {
        msg_print("Cannot close list file.");
        return;
    }

    msg_print("Successfully created a list file.");
}
