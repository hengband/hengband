#include "wizard/items-spoiler.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/trg-types.h"
#include "object/object-value.h"
#include "system/angband-system.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "wizard/spoiler-util.h"
#include <algorithm>
#include <sstream>

/*!
 * @brief アイテムの生成階層と価格を得る
 *
 * @param item アイテム
 * @return 階層と価格のペア
 */
static std::pair<DEPTH, PRICE> get_info(const ItemEntity &item)
{
    const auto level = item.get_baseitem_level();
    const auto price = item.calc_price();
    return { level, price };
}

/*!
 * @brief アイテムのダメージもしくはACの文字列表記を得る
 *
 * @param item アイテム
 * @return ダメージもしくはACの文字列表記
 */
static std::string describe_dam_or_ac(const ItemEntity &item)
{
    switch (item.bi_key.tval()) {
    case ItemKindType::SHOT:
    case ItemKindType::BOLT:
    case ItemKindType::ARROW:
    case ItemKindType::HAFTED:
    case ItemKindType::POLEARM:
    case ItemKindType::SWORD:
    case ItemKindType::DIGGING:
        return item.damage_dice.to_string();
    case ItemKindType::BOOTS:
    case ItemKindType::GLOVES:
    case ItemKindType::CLOAK:
    case ItemKindType::CROWN:
    case ItemKindType::HELM:
    case ItemKindType::SHIELD:
    case ItemKindType::SOFT_ARMOR:
    case ItemKindType::HARD_ARMOR:
    case ItemKindType::DRAG_ARMOR:
        return format("%d", item.ac);
    default:
        return {};
    }
}

/*!
 * @brief アイテムの生成確率の文字列表記を得る
 *
 * @param item アイテム
 * @return 生成確率の文字列表記
 */
static std::string describe_chance(const ItemEntity &item)
{
    std::stringstream ss;

    const auto &baseitem = item.get_baseitem();
    for (auto i = 0U; i < baseitem.alloc_tables.size(); i++) {
        const auto &[level, chance] = baseitem.alloc_tables[i];
        if (chance > 0) {
            ss << format("%s%3dF:%+4d", (i != 0 ? "/" : ""), level, 100 / chance);
        }
    }

    return ss.str();
}

/*!
 * @brief アイテムの重量の文字列表記を得る
 *
 * @param item アイテム
 * @return アイテムの重量の文字列表記
 */
static std::string describe_weight(const ItemEntity &item)
{
    return format("%3d.%d", (int)(item.weight / 10), (int)(item.weight % 10));
}

/*!
 * @brief obj-desc.txt出力用にベースアイテムIDからItemEntityオブジェクトを生成する
 * @param bi_id ベースアイテムID
 * @return obj-desc.txt出力用に使用するItemEntityオブジェクト
 * @details 人形・像・死体類はpvalが0だと異常アイテム扱いで例外が飛ぶためダミー値を入れておく.
 */
static ItemEntity prepare_item_for_obj_desc(short bi_id)
{
    ItemEntity item(bi_id);
    item.ident |= IDENT_KNOWN;
    switch (item.bi_key.tval()) {
    case ItemKindType::FIGURINE:
    case ItemKindType::STATUE:
    case ItemKindType::MONSTER_REMAINS:
        item.pval = 1;
        break;
    default:
        item.pval = 0;
        break;
    }

    item.to_a = 0;
    item.to_h = 0;
    item.to_d = 0;
    return item;
}

/*!
 * @brief 各ベースアイテムの情報を一行毎に記述する
 */
SpoilerOutputResultType spoil_obj_desc()
{
    const auto path = path_build(ANGBAND_DIR_USER, "obj-desc.txt");
    std::ofstream ofs(path);
    if (!ofs) {
        return SpoilerOutputResultType::FILE_OPEN_FAILED;
    }

    constexpr auto fmt_version = "Spoiler File -- Basic Items (%s)\n\n\n";
    ofs << format(fmt_version, AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL).data());
    ofs << format("%-37s%8s%7s%5s %40s%9s\n", "Description", "Dam/AC", "Wgt", "Lev", "Chance", "Cost");
    ofs << format("%-37s%8s%7s%5s %40s%9s\n", "-------------------------------------", "------", "---", "---", "----------------", "----");

    for (const auto &[tval_list, name] : group_item_list) {
        std::vector<short> whats;
        for (auto tval : tval_list) {
            for (const auto &baseitem : BaseitemList::get_instance()) {
                if ((baseitem.bi_key.tval() == tval) && baseitem.gen_flags.has_not(ItemGenerationTraitType::INSTA_ART)) {
                    whats.push_back(baseitem.idx);
                }
            }
        }
        if (whats.empty()) {
            continue;
        }

        std::stable_sort(whats.begin(), whats.end(), [](auto bi_id1, auto bi_id2) {
            const auto item1 = prepare_item_for_obj_desc(bi_id1);
            const auto item2 = prepare_item_for_obj_desc(bi_id2);
            const auto [depth1, price1] = get_info(item1);
            const auto [depth2, price2] = get_info(item2);
            return (price1 != price2) ? price1 < price2 : depth1 < depth2;
        });

        ofs << "\n\n"
            << name << "\n\n";
        for (const auto &bi_id : whats) {
            PlayerType dummy;
            const auto item = prepare_item_for_obj_desc(bi_id);
            const auto item_name = describe_flavor(&dummy, item, OD_NAME_ONLY | OD_STORE);
            const auto &[depth, price] = get_info(item);
            const auto dam_or_ac = describe_dam_or_ac(item);
            const auto weight = describe_weight(item);
            const auto chance = describe_chance(item);
            ofs << format("  %-35s%8s%7s%5d %-40s%9d\n", item_name.data(), dam_or_ac.data(), weight.data(), depth, chance.data(), price);
        }
    }

    return ofs.good() ? SpoilerOutputResultType::SUCCESSFUL : SpoilerOutputResultType::FILE_CLOSE_FAILED;
}
