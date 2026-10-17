/*!
 * @brief 一部職業でのみダンプする能力の出力処理
 * @date 2020/03/07
 * @author Hourier
 */

#include "io-dump/special-class-dump.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data.h"
#include "player-info/magic-eater-data-type.h"
#include "smith/object-smith.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include <algorithm>
#include <fmt/format.h>
#include <iterator>
#include <string>
#include <vector>

/*!
 * @brief 魔力喰いを持つクラスの情報をダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_magic_eater(PlayerType *player_ptr, FILE *fff)
{
    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<MagicEaterDataList>();
    if (!magic_eater_data) {
        return;
    }

    fmt::println(fff, _("\n\n  [取り込んだ魔法道具]", "\n\n  [Magic devices eaten]"));
    const auto &baseitems = BaseitemList::get_instance();
    for (auto tval : { ItemKindType::STAFF, ItemKindType::WAND, ItemKindType::ROD }) {
        switch (tval) {
        case ItemKindType::STAFF:
            fmt::println(fff, _("\n[杖]", "\n[Staffs]"));
            break;
        case ItemKindType::WAND:
            fmt::println(fff, _("\n[魔法棒]", "\n[Wands]"));
            break;
        case ItemKindType::ROD:
            fmt::println(fff, _("\n[ロッド]", "\n[Rods]"));
            break;
        default:
            break;
        }

        const auto &item_group = magic_eater_data->get_item_group(tval);
        std::vector<std::string> desc_list;
        for (auto i = 0U; i < item_group.size(); ++i) {
            auto &item = item_group[i];
            if (item.count == 0) {
                continue;
            }

            const auto &baseitem = baseitems.lookup_baseitem({ tval, i });
            const auto buf = format("%23s (%2d)", baseitem.name.data(), item.count);
            desc_list.emplace_back(buf);
        }

        if (desc_list.size() <= 0) {
            fputs(_("  (なし)\n", "  (none)\n"), fff);
            continue;
        }

        uint i;
        for (i = 0; i < desc_list.size(); i++) {
            fputs(desc_list[i].data(), fff);
            if (i % 3 < 2) {
                fputs("    ", fff);
            } else {
                fputs("\n", fff);
            }
        }

        if (i % 3 > 0) {
            fputs("\n", fff);
        }
    }
}

/*!
 * @brief 鍛冶師のエッセンス情報をダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_smith(PlayerType *player_ptr, FILE *fff)
{
    fmt::println(fff, _("\n\n  [手に入れたエッセンス]\n", "\n\n  [Get Essence]\n"));
    fmt::print(fff, _("エッセンス   個数     エッセンス   個数     エッセンス   個数", "Essence      Num      Essence      Num      Essence      Num "));

    const auto &essences = Smith::get_essence_list();
    auto n = essences.size();
    std::vector<int> amounts;
    std::transform(essences.begin(), essences.end(), std::back_inserter(amounts),
        [smith = Smith(player_ptr)](SmithEssenceType e) { return smith.get_essence_num_of_posessions(e); });

    auto row = n / 3 + 1;
    for (auto i = 0U; i < row; i++) {
        fmt::print(fff, "\n");
        fmt::print(fff, "{:<11} {:5}     ", fmt::bytes(Smith::get_essence_name(essences[i])), amounts[i]);
        if (i + row < n) {
            fmt::print(fff, "{:<11} {:5}     ", fmt::bytes(Smith::get_essence_name(essences[i + row])), amounts[i + row]);
        }
        if (i + row * 2 < n) {
            fmt::print(fff, "{:<11} {:5}", fmt::bytes(Smith::get_essence_name(essences[i + row * 2])), amounts[i + row * 2]);
        }
    }

    fputs("\n", fff);
}

/*!
 * @brief プレイヤーの職業能力情報をファイルにダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
void dump_aux_class_special(PlayerType *player_ptr, FILE *fff)
{
    switch (player_ptr->pclass) {
    case PlayerClassType::MAGIC_EATER: {
        dump_magic_eater(player_ptr, fff);
        return;
    }
    case PlayerClassType::SMITH: {
        dump_smith(player_ptr, fff);
        return;
    }
    case PlayerClassType::BLUE_MAGE: {
        const auto bluemage_data = PlayerClass(player_ptr).get_specific_data<BluemageData>();
        std::vector<std::string> lines;
        if (bluemage_data) {
            lines = bluemage_data->build_learnt_magics_info();
        }

        for (const auto &line : lines) {
            fmt::print(fff, "{}", line);
        }

        return;
    }
    default:
        return;
    }
}
