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
#include "term/z-form.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "wizard/spoiler-util.h"
#include <algorithm>

/*!
 * @brief ベースアイテムの各情報を文字列化する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param name 名称を返すバッファ参照ポインタ
 * @param damage_desc ダメージダイス記述を返すバッファ参照ポインタ
 * @param weight_desc 重量記述を返すバッファ参照ポインタ
 * @param level 生成階記述を返すバッファ参照ポインタ
 * @param chance 生成機会を返すバッファ参照ポインタ
 * @param value 価値を返すバッファ参照ポインタ
 * @param bi_id ベースアイテムID
 */
static void describe_baseitem_info(PlayerType *player_ptr,
    char *name, std::string *damage_desc, std::string *weight_desc, std::string *chance_desc, DEPTH *level_desc, PRICE *value, short bi_id)
{
    ItemEntity forge;
    auto *q_ptr = &forge;
    q_ptr->prep(bi_id);
    q_ptr->ident |= IDENT_KNOWN;
    q_ptr->pval = 0;
    q_ptr->to_a = 0;
    q_ptr->to_h = 0;
    q_ptr->to_d = 0;
    *level_desc = baseitems_info[q_ptr->bi_id].level;
    *value = q_ptr->get_price();
    if (!name || !damage_desc || !chance_desc || !weight_desc) {
        return;
    }

    auto item_name = describe_flavor(player_ptr, q_ptr, OD_NAME_ONLY | OD_STORE);
    name = item_name.data();
    switch (q_ptr->bi_key.tval()) {
    case ItemKindType::SHOT:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
        *damage_desc = format("%dd%d", q_ptr->dd, q_ptr->ds);
        break;
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
        *damage_desc = format("%dd%d", q_ptr->dd, q_ptr->ds);
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
        *damage_desc = format("%d", q_ptr->ac);
        break;
    default:
        damage_desc->clear();
        break;
    }

    chance_desc->clear();
    const auto &baseitem = baseitems_info[q_ptr->bi_id];
    for (auto i = 0U; i < baseitem.alloc_tables.size(); i++) {
        const auto &[level, chance] = baseitem.alloc_tables[i];
        if (chance > 0) {
            chance_desc->append(format("%s%3dF:%+4d", (i != 0 ? "/" : ""), level, 100 / chance));
        }
    }

    *weight_desc = format("%3d.%d", (int)(q_ptr->weight / 10), (int)(q_ptr->weight % 10));
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
    spoiler_file = angband_fopen(buf, FileOpenMode::WRITE);
    if (!spoiler_file) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    fprintf(spoiler_file, "Spoiler File -- Basic Items (%s)\n\n\n", get_version().data());
    fprintf(spoiler_file, "%-37s%8s%7s%5s %40s%9s\n", "Description", "Dam/AC", "Wgt", "Lev", "Chance", "Cost");
    fprintf(spoiler_file, "%-37s%8s%7s%5s %40s%9s\n", "-------------------------------------", "------", "---", "---", "----------------", "----");

    for (const auto &[tval_list, name] : group_item_list) {
        std::vector<short> whats;
        for (auto tval : tval_list) {
            for (const auto &baseitem : baseitems_info) {
                if ((baseitem.bi_key.tval() == tval) && baseitem.gen_flags.has_not(ItemGenerationTraitType::INSTA_ART)) {
                    whats.push_back(baseitem.idx);
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
            describe_baseitem_info(&dummy, nullptr, nullptr, nullptr, nullptr, &d1, &p1, k1_idx);
            describe_baseitem_info(&dummy, nullptr, nullptr, nullptr, nullptr, &d2, &p2, k2_idx);
            return (p1 != p2) ? p1 < p2 : d1 < d2;
        });

        fprintf(spoiler_file, "\n\n%s\n\n", name);
        for (const auto &bi_id : whats) {
            DEPTH e;
            PRICE v;
            std::string wgt, chance, dam;
            PlayerType dummy;
            describe_baseitem_info(&dummy, buf, &dam, &wgt, &chance, &e, &v, bi_id);
            fprintf(spoiler_file, "  %-35s%8s%7s%5d %-40s%9ld\n", buf,
                dam.data(), wgt.data(), static_cast<int>(e), chance.data(),
                static_cast<long>(v));
        }
    }

    return ferror(spoiler_file) || angband_fclose(spoiler_file) ? SpoilerOutputResultType::FILE_CLOSE_FAILED
                                                                : SpoilerOutputResultType::SUCCESSFUL;
}
