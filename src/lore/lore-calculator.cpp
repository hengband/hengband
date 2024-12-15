#include "lore/lore-calculator.h"
#include "game-option/cheat-options.h"
#include "lore/lore-util.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "util/string-processor.h"
#include <sstream>

/*!
 * @brief ダイス目を文字列に変換する
 * @param base_damage 固定値
 * @param dice_num ダイス数
 * @param dice_side ダイス面
 * @param dice_mult ダイス倍率
 * @param dice_div ダイス除数
 * @return std::string サイコロ式の印刷可能なバージョン
 */
std::string dice_to_string(int base_damage, int dice_num, int dice_side, int dice_mult, int dice_div)
{
    if (dice_num == 0) {
        return std::to_string(base_damage);
    }

    std::stringstream ss;
    if (base_damage != 0) {
        ss << base_damage << '+';
    }

    if (dice_num != 1) {
        ss << dice_num;
    }
    ss << 'd' << dice_side;

    if (dice_mult != 1 || dice_div != 1) {
        ss << '*';
        if (dice_div == 1) {
            ss << dice_mult;
        } else {
            ss << '(' << dice_mult << '/' << dice_div << ')';
        }
    }

    return ss.str();
}

/*!
 * @brief lore_ptrにダメージを与えるスキルの情報を追加する
 * @param lore_ptr 知識情報
 * @param ms_type スキル
 * @param msg スキルを表す文字列
 * @param color 表示する文字色
 */
void add_lore_of_damage_skill(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type, concptr msg, byte color)
{
    if (!lore_ptr->is_details_known() && !lore_ptr->know_everything) {
        // ダメージ量の情報なし
        lore_ptr->lore_msgs.emplace_back(format(msg, ""), color);
        return;
    }

    const auto monrace_id = lore_ptr->monrace_id;
    const auto base_damage = monspell_race_damage(player_ptr, ms_type, monrace_id, BASE_DAM);
    const auto dice_num = monspell_race_damage(player_ptr, ms_type, monrace_id, DICE_NUM);
    const auto dice_side = monspell_race_damage(player_ptr, ms_type, monrace_id, DICE_SIDE);
    const auto dice_mult = monspell_race_damage(player_ptr, ms_type, monrace_id, DICE_MULT);
    const auto dice_div = monspell_race_damage(player_ptr, ms_type, monrace_id, DICE_DIV);
    std::stringstream dam_info;
    dam_info << '(' << dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div) << ')';
    lore_ptr->lore_msgs.emplace_back(format(msg, dam_info.str().data()), color);
}

void set_flags_for_full_knowledge(lore_type *lore_ptr)
{
    if (!lore_ptr->know_everything) {
        return;
    }

    lore_ptr->drop_gold = lore_ptr->drop_item = ((lore_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_4D2) ? 8 : 0) + (lore_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_3D2) ? 6 : 0) + (lore_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_2D2) ? 4 : 0) + (lore_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_1D2) ? 2 : 0) + (lore_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_90) ? 1 : 0) + (lore_ptr->r_ptr->drop_flags.has(MonsterDropType::DROP_60) ? 1 : 0));

    if (lore_ptr->r_ptr->drop_flags.has(MonsterDropType::ONLY_GOLD)) {
        lore_ptr->drop_item = 0;
    }

    if (lore_ptr->r_ptr->drop_flags.has(MonsterDropType::ONLY_ITEM)) {
        lore_ptr->drop_gold = 0;
    }

    lore_ptr->ability_flags = lore_ptr->r_ptr->ability_flags;
    lore_ptr->aura_flags = lore_ptr->r_ptr->aura_flags;
    lore_ptr->behavior_flags = lore_ptr->r_ptr->behavior_flags;
    lore_ptr->resistance_flags = lore_ptr->r_ptr->resistance_flags;
    lore_ptr->feature_flags = lore_ptr->r_ptr->feature_flags;
    lore_ptr->drop_flags = lore_ptr->r_ptr->drop_flags;
    lore_ptr->special_flags = lore_ptr->r_ptr->special_flags;
    lore_ptr->misc_flags = lore_ptr->r_ptr->misc_flags;
}
