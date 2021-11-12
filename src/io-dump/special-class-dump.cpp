/*!
 * @brief 一部職業でのみダンプする能力の出力処理
 * @date 2020/03/07
 * @author Hourier
 */

#include "io-dump/special-class-dump.h"
#include "blue-magic/blue-magic-checker.h"
#include "cmd-item/cmd-magiceat.h"
#include "mind/mind-blue-mage.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/monster-power-table.h"
#include "object-enchant/object-smith.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "player-base/player-class.h"
#include "player-info/bluemage-data-type.h"
#include "player-info/magic-eater-data-type.h"
#include "system/player-type-definition.h"
#include "util/enum-converter.h"
#include "util/flag-group.h"

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

typedef struct {
    EnumClassFlagGroup<MonsterAbilityType> ability_flags;
} learnt_spell_table;

/*!
 * @brief 魔力喰いを持つクラスの情報をダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_magic_eater(PlayerType *player_ptr, FILE *fff)
{
    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<magic_eater_data_type>();
    if (!magic_eater_data) {
        return;
    }

    fprintf(fff, _("\n\n  [取り込んだ魔法道具]\n", "\n\n  [Magic devices eaten]\n"));

    for (auto tval : { ItemKindType::STAFF, ItemKindType::WAND, ItemKindType::ROD }) {
        switch (tval) {
        case ItemKindType::STAFF:
            fprintf(fff, _("\n[杖]\n", "\n[Staffs]\n"));
            break;
        case ItemKindType::WAND:
            fprintf(fff, _("\n[魔法棒]\n", "\n[Wands]\n"));
            break;
        case ItemKindType::ROD:
            fprintf(fff, _("\n[ロッド]\n", "\n[Rods]\n"));
            break;
        default:
            break;
        }

        const auto &item_group = magic_eater_data->get_item_group(tval);
        std::vector<std::string> desc_list;
        for (auto i = 0U; i < item_group.size(); ++i) {
            auto &item = item_group[i];
            if (item.count == 0)
                continue;

            KIND_OBJECT_IDX k_idx = lookup_kind(tval, i);
            if (!k_idx)
                continue;

            char buf[128];
            snprintf(buf, sizeof(buf), "%23s (%2d)", k_info[k_idx].name.c_str(), item.count);
            desc_list.emplace_back(buf);
        }

        if (desc_list.size() <= 0) {
            fputs(_("  (なし)\n", "  (none)\n"), fff);
            continue;
        }

        uint i;
        for (i = 0; i < desc_list.size(); i++) {
            fputs(desc_list[i].c_str(), fff);
            if (i % 3 < 2)
                fputs("    ", fff);
            else
                fputs("\n", fff);
        }

        if (i % 3 > 0)
            fputs("\n", fff);
    }
}

/*!
 * @brief 鍛冶師のエッセンス情報をダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_smith(PlayerType *player_ptr, FILE *fff)
{
    fprintf(fff, _("\n\n  [手に入れたエッセンス]\n\n", "\n\n  [Get Essence]\n\n"));
    fprintf(fff, _("エッセンス   個数     エッセンス   個数     エッセンス   個数", "Essence      Num      Essence      Num      Essence      Num "));

    auto essences = Smith::get_essence_list();
    auto n = essences.size();
    std::vector<int> amounts;
    std::transform(essences.begin(), essences.end(), std::back_inserter(amounts),
        [smith = Smith(player_ptr)](SmithEssenceType e) { return smith.get_essence_num_of_posessions(e); });

    auto row = n / 3 + 1;
    for (auto i = 0U; i < row; i++) {
        fprintf(fff, "\n");
        fprintf(fff, "%-11s %5d     ", Smith::get_essence_name(essences[i]), amounts[i]);
        if (i + row < n)
            fprintf(fff, "%-11s %5d     ", Smith::get_essence_name(essences[i + row]), amounts[i + row]);
        if (i + row * 2 < n)
            fprintf(fff, "%-11s %5d", Smith::get_essence_name(essences[i + row * 2]), amounts[i + row * 2]);
    }

    fputs("\n", fff);
}

/*!
 * @brief ダンプする情報に学習済魔法の種類を追加する
 * @param p ダンプ用のバッファ
 * @param col 行数
 * @param SpellProcessType 魔法の種類
 * @param learnt_spell_ptr 学習済魔法のテーブル
 */
static void add_monster_spell_type(char p[][80], int col, BlueMagicType SpellProcessType, learnt_spell_table *learnt_spell_ptr)
{
    learnt_spell_ptr->ability_flags.clear();
    set_rf_masks(learnt_spell_ptr->ability_flags, SpellProcessType);
    switch (SpellProcessType) {
    case BlueMagicType::BOLT:
        strcat(p[col], _("\n     [ボルト型]\n", "\n     [Bolt  Type]\n"));
        break;

    case BlueMagicType::BALL:
        strcat(p[col], _("\n     [ボール型]\n", "\n     [Ball  Type]\n"));
        break;

    case BlueMagicType::BREATH:
        strcat(p[col], _("\n     [ブレス型]\n", "\n     [  Breath  ]\n"));
        break;

    case BlueMagicType::SUMMON:
        strcat(p[col], _("\n     [召喚魔法]\n", "\n     [Summonning]\n"));
        break;

    case BlueMagicType::OTHER:
        strcat(p[col], _("\n     [ その他 ]\n", "\n     [Other Type]\n"));
        break;
    }
}

/*!
 * @brief 青魔道士の学習済魔法をダンプする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff ファイルポインタ
 */
static void dump_blue_mage(PlayerType *player_ptr, FILE *fff)
{
    const auto bluemage_data = PlayerClass(player_ptr).get_specific_data<bluemage_data_type>();
    if (!bluemage_data) {
        return;
    }

    char p[60][80];
    for (int i = 0; i < 60; i++) {
        p[i][0] = '\0';
    }

    int col = 0;
    strcat(p[col], _("\n\n  [学習済みの青魔法]\n", "\n\n  [Learned Blue Magic]\n"));

    for (auto SpellProcessType : BLUE_MAGIC_TYPE_LIST) {
        col++;
        learnt_spell_table learnt_magic;
        add_monster_spell_type(p, col, SpellProcessType, &learnt_magic);
        learnt_magic.ability_flags &= bluemage_data->learnt_blue_magics;

        std::vector<MonsterAbilityType> learnt_spells;
        EnumClassFlagGroup<MonsterAbilityType>::get_flags(learnt_magic.ability_flags, std::back_inserter(learnt_spells));

        col++;
        bool pcol = false;
        strcat(p[col], "       ");

        for (auto spell : learnt_spells) {
            pcol = true;
            int l1 = strlen(p[col]);
            int l2 = strlen(monster_powers_short.at(spell));
            if ((l1 + l2) >= 75) {
                strcat(p[col], "\n");
                col++;
                strcat(p[col], "       ");
            }

            strcat(p[col], monster_powers_short.at(spell));
            strcat(p[col], ", ");
        }

        if (!pcol) {
            strcat(p[col], _("なし", "None"));
            strcat(p[col], "\n");
            continue;
        }

        if (p[col][strlen(p[col]) - 2] == ',') {
            p[col][strlen(p[col]) - 2] = '\0';
        } else {
            p[col][strlen(p[col]) - 10] = '\0';
        }

        strcat(p[col], "\n");
    }

    for (int i = 0; i <= col; i++) {
        fputs(p[i], fff);
    }
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
        dump_blue_mage(player_ptr, fff);
        return;
    }
    default:
        return;
    }
}
