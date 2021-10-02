#include "io-dump/random-art-info-dumper.h"
#include "floor/floor-town.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "perception/object-perception.h"
#include "store/store-util.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "wizard/artifact-analyzer.h"
#include "wizard/fixed-artifacts-spoiler.h"
#include "wizard/spoiler-util.h"

/*!
 * @brief ランダムアーティファクト1件をスポイラー出力する /
 * Create a spoiler file entry for an artifact
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param art_ptr 記述内容を収めた構造体参照ポインタ
 * Fill in an object description structure for a given object
 */
static void spoiler_print_randart(object_type *o_ptr, obj_desc_list *art_ptr)
{
    pval_info_type *pval_ptr = &art_ptr->pval_info;
    char buf[80];
    fprintf(spoiler_file, "%s\n", art_ptr->description);
    if (!o_ptr->is_fully_known()) {
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトのオブジェクト構造体参照ポインタ
 * @param tval 出力したいランダムアーティファクトの種類
 */
static void spoil_random_artifact_aux(player_type *player_ptr, object_type *o_ptr, tval_type tval)
{
    obj_desc_list artifact;
    if (!o_ptr->is_known() || !o_ptr->art_name || o_ptr->tval != tval)
        return;

    random_artifact_analyze(player_ptr, o_ptr, &artifact);
    spoiler_print_randart(o_ptr, &artifact);
}

/*!
 * @brief ランダムアーティファクト内容をスポイラー出力するメインルーチン /
 * Create a list file for random artifacts
 * @param fname 出力ファイル名
 */
void spoil_random_artifact(player_type *player_ptr, concptr fname)
{
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
    for (const auto &[tval_list, name] : group_artifact_list) {
        for (auto tval : tval_list) {
            for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
                q_ptr = &player_ptr->inventory_list[i];
                spoil_random_artifact_aux(player_ptr, q_ptr, tval);
            }

            for (int i = 0; i < INVEN_PACK; i++) {
                q_ptr = &player_ptr->inventory_list[i];
                spoil_random_artifact_aux(player_ptr, q_ptr, tval);
            }

            store_ptr = &town_info[1].store[STORE_HOME];
            for (int i = 0; i < store_ptr->stock_num; i++) {
                q_ptr = &store_ptr->stock[i];
                spoil_random_artifact_aux(player_ptr, q_ptr, tval);
            }

            store_ptr = &town_info[1].store[STORE_MUSEUM];
            for (int i = 0; i < store_ptr->stock_num; i++) {
                q_ptr = &store_ptr->stock[i];
                spoil_random_artifact_aux(player_ptr, q_ptr, tval);
            }
        }
    }

    if (ferror(spoiler_file) || angband_fclose(spoiler_file)) {
        msg_print("Cannot close list file.");
        return;
    }

    msg_print("Successfully created a list file.");
}
