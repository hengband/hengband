#include "wizard/items-spoiler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trg-types.h"
#include "object/object-value.h"
#include "system/angband-version.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "wizard/spoiler-util.h"

#include <algorithm>

/*!
 * @brief ベースアイテムの各情報を文字列化する /
 * Describe the kind
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param buf 名称を返すバッファ参照ポインタ
 * @param dam ダメージダイス記述を返すバッファ参照ポインタ
 * @param wgt 重量記述を返すバッファ参照ポインタ
 * @param lev 生成階記述を返すバッファ参照ポインタ
 * @param chance 生成機会を返すバッファ参照ポインタ
 * @param val 価値を返すバッファ参照ポインタ
 * @param k ベースアイテムID
 */
static void kind_info(PlayerType *player_ptr, char *buf, char *dam, char *wgt, char *chance, DEPTH *lev, PRICE *val, short k)
{
    ItemEntity forge;
    auto *q_ptr = &forge;
    q_ptr->prep(k);
    q_ptr->ident |= IDENT_KNOWN;
    q_ptr->pval = 0;
    q_ptr->to_a = 0;
    q_ptr->to_h = 0;
    q_ptr->to_d = 0;
    *lev = baseitems_info[q_ptr->bi_id].level;
    *val = q_ptr->get_price();
    if (!buf || !dam || !chance || !wgt) {
        return;
    }

    describe_flavor(player_ptr, buf, q_ptr, OD_NAME_ONLY | OD_STORE);
    strcpy(dam, "");
    switch (q_ptr->bi_key.tval()) {
    case ItemKindType::SHOT:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
        sprintf(dam, "%dd%d", q_ptr->dd, q_ptr->ds);
        break;
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
        sprintf(dam, "%dd%d", q_ptr->dd, q_ptr->ds);
        break;
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::CLOAK:
    case ItemKindType::CROWN:
    case ItemKindType::HELM:
    case ItemKindType::SHIELD:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
        sprintf(dam, "%d", q_ptr->ac);
        break;
    default:
        break;
    }

    strcpy(chance, "");
    for (int i = 0; i < 4; i++) {
        char chance_aux[20] = "";
        if (baseitems_info[q_ptr->bi_id].chance[i] > 0) {
            sprintf(chance_aux, "%s%3dF:%+4d", (i != 0 ? "/" : ""), (int)baseitems_info[q_ptr->bi_id].locale[i], 100 / baseitems_info[q_ptr->bi_id].chance[i]);
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
SpoilerOutputResultType spoil_obj_desc(concptr fname)
{
    char buf[1024];
    path_build(buf, sizeof(buf), ANGBAND_DIR_USER, fname);
    spoiler_file = angband_fopen(buf, "w");
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    char title[200];
    put_version(title);
    fprintf(spoiler_file, "Spoiler File -- Basic Items (%s)\n\n\n", title);
    fprintf(spoiler_file, "%-37s%8s%7s%5s %40s%9s\n", "Description", "Dam/AC", "Wgt", "Lev", "Chance", "Cost");
    fprintf(spoiler_file, "%-37s%8s%7s%5s %40s%9s\n", "-------------------------------------", "------", "---", "---", "----------------", "----");

    for (const auto &[tval_list, name] : group_item_list) {
        std::vector<short> whats;
        for (auto tval : tval_list) {
            for (const auto &k_ref : baseitems_info) {
                if ((k_ref.bi_key.tval() == tval) && k_ref.gen_flags.has_not(ItemGenerationTraitType::INSTA_ART)) {
                    whats.push_back(k_ref.idx);
                }
            }
        }
        if (whats.empty()) {
            continue;
        }

        std::stable_sort(whats.begin(), whats.end(), [](auto k1_idx, auto k2_idx) {
            PlayerType dummy;
            DEPTH d1, d2;
            PRICE p1, p2;
            kind_info(&dummy, nullptr, nullptr, nullptr, nullptr, &d1, &p1, k1_idx);
            kind_info(&dummy, nullptr, nullptr, nullptr, nullptr, &d2, &p2, k2_idx);
            return (p1 != p2) ? p1 < p2 : d1 < d2;
        });

        fprintf(spoiler_file, "\n\n%s\n\n", name);
        for (const auto &bi_id : whats) {
            DEPTH e;
            PRICE v;
            char wgt[80];
            char chance[80];
            char dam[80];
            PlayerType dummy;
            kind_info(&dummy, buf, dam, wgt, chance, &e, &v, bi_id);
            fprintf(spoiler_file, "  %-35s%8s%7s%5d %-40s%9ld\n", buf, dam, wgt, static_cast<int>(e), chance, static_cast<long>(v));
        }
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}
