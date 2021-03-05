﻿#include "wizard/fixed-artifacts-spoiler.h"
#include "io/files-util.h"
#include "object/object-generator.h"
#include "object/object-kind-hook.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/object-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "wizard/artifact-analyzer.h"
#include "wizard/spoiler-util.h"

/*!
 * todo 固定アーティファクトとランダムアーティファクトで共用、ここに置くべきかは要調整.
 * @brief フラグ名称を出力する汎用関数
 * @param header ヘッダに出力するフラグ群の名前
 * @param list フラグ名リスト
 * @param separator フラグ表示の区切り記号
 * @return なし
 */
void spoiler_outlist(concptr header, concptr *list, char separator)
{
    char line[MAX_LINE_LEN + 20], buf[80];
    if (*list == NULL)
        return;

    strcpy(line, spoiler_indent);
    if (header && (header[0])) {
        strcat(line, header);
        strcat(line, " ");
    }

    int buf_len;
    int line_len = strlen(line);
    while (TRUE) {
        strcpy(buf, *list);
        buf_len = strlen(buf);
        if (list[1]) {
            sprintf(buf + buf_len, "%c ", separator);
            buf_len += 2;
        }

        if (line_len + buf_len <= MAX_LINE_LEN) {
            strcat(line, buf);
            line_len += buf_len;
        } else {
            if (line_len > 1 && line[line_len - 1] == ' ' && line[line_len - 2] == list_separator) {
                line[line_len - 2] = '\0';
                fprintf(spoiler_file, "%s\n", line);
                sprintf(line, "%s%s", spoiler_indent, buf);
            } else {
                fprintf(spoiler_file, "%s\n", line);
                concptr ident2 = "      ";
                sprintf(line, "%s%s", ident2, buf);
            }

            line_len = strlen(line);
        }

        if (!*++list)
            break;
    }

    fprintf(spoiler_file, "%s\n", line);
}

/*!
 * @brief バッファにアーティファクト出力情報ヘッダを収める /
 * @return なし
 */
static void print_header(void)
{
    char buf[360];
    char title[180];
    put_version(title);
    sprintf(buf, "Artifact Spoilers for Hengband Version %s", title);
    spoiler_underline(buf);
}

/*!
 * @brief アーティファクト情報を出力するためにダミー生成を行う /
 * Hack -- Create a "forged" artifact
 * @param o_ptr 一時生成先を保管するオブジェクト構造体
 * @param name1 生成するアーティファクトID
 * @return 生成が成功した場合TRUEを返す
 */
static bool make_fake_artifact(player_type *player_ptr, object_type *o_ptr, ARTIFACT_IDX name1)
{
    artifact_type *a_ptr = &a_info[name1];
    if (!a_ptr->name)
        return FALSE;

    OBJECT_IDX i = lookup_kind(a_ptr->tval, a_ptr->sval);
    if (!i)
        return FALSE;

    object_prep(player_ptr, o_ptr, i);
    o_ptr->name1 = name1;
    o_ptr->pval = a_ptr->pval;
    o_ptr->ac = a_ptr->ac;
    o_ptr->dd = a_ptr->dd;
    o_ptr->ds = a_ptr->ds;
    o_ptr->to_a = a_ptr->to_a;
    o_ptr->to_h = a_ptr->to_h;
    o_ptr->to_d = a_ptr->to_d;
    o_ptr->weight = a_ptr->weight;
    return TRUE;
}

/*!
 * @brief アーティファクト一件をスポイラー出力する /
 * Create a spoiler file entry for an artifact
 * @param art_ptr アーティファクト情報をまとめた構造体の参照ポインタ
 * @return なし
 */
static void spoiler_print_art(obj_desc_list *art_ptr)
{
    pval_info_type *pval_ptr = &art_ptr->pval_info;
    char buf[80];
    fprintf(spoiler_file, "%s\n", art_ptr->description);
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

    if (art_ptr->addition[0])
        fprintf(spoiler_file, _("%s追加: %s\n", "%sAdditional %s\n"), spoiler_indent, art_ptr->addition);

    if (art_ptr->activation)
        fprintf(spoiler_file, _("%s発動: %s\n", "%sActivates for %s\n"), spoiler_indent, art_ptr->activation);

    fprintf(spoiler_file, "%s%s\n\n", spoiler_indent, art_ptr->misc_desc);
}

/*!
 * @brief アーティファクト情報のスポイラー出力を行うメインルーチン /
 * Create a spoiler file for artifacts
 * @param fname 生成ファイル名
 */
spoiler_output_status spoil_fixed_artifact(concptr fname)
{
    player_type dummy;
    object_type forge;
    object_type *q_ptr;
    obj_desc_list artifact;
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return SPOILER_OUTPUT_FAIL_FOPEN;
    }

    print_header();
    for (int i = 0; group_artifact[i].tval; i++) {
        if (group_artifact[i].name) {
            spoiler_blanklines(2);
            spoiler_underline(group_artifact[i].name);
            spoiler_blanklines(1);
        }

        for (ARTIFACT_IDX j = 1; j < max_a_idx; ++j) {
            artifact_type *a_ptr = &a_info[j];
            if (a_ptr->tval != group_artifact[i].tval)
                continue;

            q_ptr = &forge;
            object_wipe(q_ptr);
            if (!make_fake_artifact(&dummy, q_ptr, j))
                continue;

            object_analyze(&dummy, q_ptr, &artifact);
            spoiler_print_art(&artifact);
        }
    }

    if (ferror(spoiler_file) || angband_fclose(spoiler_file)) {
        return SPOILER_OUTPUT_FAIL_FCLOSE;
    }
    return SPOILER_OUTPUT_SUCCESS;
}
