#include "lore/lore-calculator.h"
#include "lore/lore-util.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include <fmt/format.h>

/*!
 * @brief モンスターのスキルのダメージを表す文字列を得る
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param lore_ptr 知識情報
 * @param ms_type スキル
 * @return 括弧で囲んだダメージ (固定値か、ダイスを含む式。例: "(50)"、"(10+2d6)")
 */
std::string get_skill_damage(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type)
{
    const auto monrace_id = lore_ptr->monrace_id;
    const auto base_damage = monspell_race_damage(player_ptr, ms_type, monrace_id, BASE_DAM);
    const auto dice_num = monspell_race_damage(player_ptr, ms_type, monrace_id, DICE_NUM);
    const auto dice_side = monspell_race_damage(player_ptr, ms_type, monrace_id, DICE_SIDE);
    const auto dice_mult = monspell_race_damage(player_ptr, ms_type, monrace_id, DICE_MULT);
    const auto dice_div = monspell_race_damage(player_ptr, ms_type, monrace_id, DICE_DIV);
    return fmt::format("({})", dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div));
}

/*!
 * @brief lore_ptrにダメージを与えるスキルの情報を追加する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param lore_ptr 知識情報
 * @param ms_type スキル
 * @param msg スキルを表す文字列の書式 ({} にダメージの情報が入る)
 * @param color 表示する文字色
 */
void add_lore_of_damage_skill(PlayerType *player_ptr, lore_type *lore_ptr, MonsterAbilityType ms_type, fmt::format_string<std::string_view> msg, byte color)
{
    if (!lore_ptr->is_details_known() && !lore_ptr->know_everything) {
        // ダメージ量の情報なし
        lore_ptr->lore_msgs.emplace_back(fmt::format(msg, std::string_view()), color);
        return;
    }

    lore_ptr->lore_msgs.emplace_back(fmt::format(msg, std::string_view(get_skill_damage(player_ptr, lore_ptr, ms_type))), color);
}

void set_flags_for_full_knowledge(lore_type *lore_ptr)
{
    if (!lore_ptr->know_everything) {
        return;
    }

    lore_ptr->drop_gold = lore_ptr->drop_item = ((lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_4D2) ? 8 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_3D2) ? 6 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_2D2) ? 4 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_1D2) ? 2 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_90) ? 1 : 0) + (lore_ptr->monrace->drop_flags.has(MonsterDropType::DROP_60) ? 1 : 0));

    if (lore_ptr->monrace->drop_flags.has(MonsterDropType::ONLY_GOLD)) {
        lore_ptr->drop_item = 0;
    }

    if (lore_ptr->monrace->drop_flags.has(MonsterDropType::ONLY_ITEM)) {
        lore_ptr->drop_gold = 0;
    }

    lore_ptr->ability_flags = lore_ptr->monrace->ability_flags;
    lore_ptr->aura_flags = lore_ptr->monrace->aura_flags;
    lore_ptr->behavior_flags = lore_ptr->monrace->behavior_flags;
    lore_ptr->resistance_flags = lore_ptr->monrace->resistance_flags;
    lore_ptr->feature_flags = lore_ptr->monrace->feature_flags;
    lore_ptr->drop_flags = lore_ptr->monrace->drop_flags;
    lore_ptr->special_flags = lore_ptr->monrace->special_flags;
    lore_ptr->misc_flags = lore_ptr->monrace->misc_flags;
}
