#include "wizard/items-spoiler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trg-types.h"
#include "object/object-kind.h"
#include "object/object-value.h"
#include "system/angband-version.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "wizard/spoiler-util.h"

/*!
 * @brief ベースアイテムの各情報を文字列化する /
 * Describe the kind
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param buf 名称を返すバッファ参照ポインタ
 * @param dam ダメージダイス記述を返すバッファ参照ポインタ
 * @param wgt 重量記述を返すバッファ参照ポインタ
 * @param lev 生成階記述を返すバッファ参照ポインタ
 * @param chance 生成機会を返すバッファ参照ポインタ
 * @param val 価値を返すバッファ参照ポインタ
 * @param k ベースアイテムID
 */
static void kind_info(player_type *player_ptr, char *buf, char *dam, char *wgt, char *chance, DEPTH *lev, PRICE *val, OBJECT_IDX k)
{
    object_type forge;
    object_type *q_ptr = &forge;
    q_ptr->prep(player_ptr, k);
    q_ptr->ident |= IDENT_KNOWN;
    q_ptr->pval = 0;
    q_ptr->to_a = 0;
    q_ptr->to_h = 0;
    q_ptr->to_d = 0;
    *lev = k_info[q_ptr->k_idx].level;
    *val = object_value(player_ptr, q_ptr);
    if (!buf || !dam || !chance || !wgt)
        return;

    describe_flavor(player_ptr, buf, q_ptr, OD_NAME_ONLY | OD_STORE);
    strcpy(dam, "");
    switch (q_ptr->tval) {
    case TV_SHOT:
    case TV_BOLT:
    case TV_ARROW:
        sprintf(dam, "%dd%d", q_ptr->dd, q_ptr->ds);
        break;
    case TV_HAFTED:
    case TV_POLEARM:
    case TV_SWORD:
    case TV_DIGGING:
        sprintf(dam, "%dd%d", q_ptr->dd, q_ptr->ds);
        break;
    case TV_BOOTS:
    case TV_GLOVES:
    case TV_CLOAK:
    case TV_CROWN:
    case TV_HELM:
    case TV_SHIELD:
    case TV_SOFT_ARMOR:
    case TV_HARD_ARMOR:
    case TV_DRAG_ARMOR:
        sprintf(dam, "%d", q_ptr->ac);
        break;
    default:
        break;
    }

    strcpy(chance, "");
    for (int i = 0; i < 4; i++) {
        char chance_aux[20] = "";
        if (k_info[q_ptr->k_idx].chance[i] > 0) {
            sprintf(chance_aux, "%s%3dF:%+4d", (i != 0 ? "/" : ""), (int)k_info[q_ptr->k_idx].locale[i], 100 / k_info[q_ptr->k_idx].chance[i]);
            strcat(chance, chance_aux);
        }
    }

    sprintf(wgt, "%3d.%d", (int)(q_ptr->weight / 10), (int)(q_ptr->weight % 10));
}

/*!
 * @brief 各ベースアイテムの情報を一行毎に記述する /
 * Create a spoiler file for items
 * @param fname ファイル名
 */
spoiler_output_status spoil_obj_desc(concptr fname)
{
    player_type dummy;
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return spoiler_output_status::SPOILER_OUTPUT_FAIL_FOPEN;
    }

    char title[200];
    put_version(title);
    fprintf(spoiler_file, "Spoiler File -- Basic Items (%s)\n\n\n", title);
    fprintf(spoiler_file, "%-37s%8s%7s%5s %40s%9s\n", "Description", "Dam/AC", "Wgt", "Lev", "Chance", "Cost");
    fprintf(spoiler_file, "%-37s%8s%7s%5s %40s%9s\n", "-------------------------------------", "------", "---", "---", "----------------", "----");
    int n = 0;
    int group_start = 0;
    OBJECT_IDX who[200];
    for (int i = 0; true; i++) {
        if (group_item[i].name) {
            if (n) {
                for (int s = 0; s < n - 1; s++) {
                    for (int t = 0; t < n - 1; t++) {
                        int i1 = t;
                        int i2 = t + 1;

                        DEPTH e1;
                        DEPTH e2;

                        PRICE t1;
                        PRICE t2;

                        kind_info(&dummy, NULL, NULL, NULL, NULL, &e1, &t1, who[i1]);
                        kind_info(&dummy, NULL, NULL, NULL, NULL, &e2, &t2, who[i2]);

                        if ((t1 > t2) || ((t1 == t2) && (e1 > e2))) {
                            uint16_t tmp = who[i1];
                            who[i1] = who[i2];
                            who[i2] = tmp;
                        }
                    }
                }

                fprintf(spoiler_file, "\n\n%s\n\n", group_item[group_start].name);
                for (int s = 0; s < n; s++) {
                    DEPTH e;
                    PRICE v;
                    char wgt[80];
                    char chance[80];
                    char dam[80];
                    kind_info(&dummy, buf, dam, wgt, chance, &e, &v, who[s]);
                    fprintf(spoiler_file, "  %-35s%8s%7s%5d %-40s%9ld\n", buf, dam, wgt, (int)e, chance, (long)(v));
                }

                n = 0;
            }

            if (!group_item[i].tval)
                break;

            group_start = i;
        }

        for (int k = 1; k < max_k_idx; k++) {
            object_kind *k_ptr = &k_info[k];
            if ((k_ptr->tval != group_item[i].tval) || k_ptr->gen_flags.has(TRG::INSTA_ART))
                continue;

            who[n++] = (uint16_t)k;
        }
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? spoiler_output_status::SPOILER_OUTPUT_FAIL_FCLOSE
                                                                : spoiler_output_status::SPOILER_OUTPUT_SUCCESS;
}
