#include "lore/lore-calculator.h"
#include "game-option/cheat-options.h"
#include "lore/lore-util.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "system/monster-race-info.h"
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
 * @brief モンスターの詳細情報(HP,AC,スキルダメージの量)を得ることができるかを返す
 * @param r_idx モンスターの種族ID
 * @return モンスターの詳細情報を得る条件が満たされているならtrue、そうでないならfalse
 * @details
 * The higher the level, the fewer kills needed.
 */
bool know_details(MonsterRaceId r_idx)
{
    const auto &monrace = monraces_info[r_idx];
    const auto level = monrace.level;
    const auto kills = monrace.r_tkills;

    if (monrace.r_cast_spell == MAX_UCHAR) {
        return true;
    }
    if (kills > 304 / (4 + level)) {
        return true;
    }
    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }
    if (kills > 304 / (38 + (5 * level) / 4)) {
        return true;
    }
    return false;
}

/*!
 * @brief モンスターの打撃威力を知ることができるかどうかを返す
 * Determine if the "damage" of the given attack is known
 * @param r_idx モンスターの種族ID
 * @param i 確認したい攻撃手番
 * @return 敵のダメージダイスを知る条件が満たされているならtrue、そうでないならfalse
 * @details
 * <pre>
 * the higher the level of the monster, the fewer the attacks you need,
 * the more damage an attack does, the more attacks you need
 * </pre>
 */
bool know_blow_damage(MonsterRaceId r_idx, int i)
{
    const auto &monrace = monraces_info[r_idx];
    auto level = monrace.level;
    auto a = monrace.r_blows[i];
    auto d1 = monrace.blows[i].d_dice;
    auto d2 = monrace.blows[i].d_side;
    auto d = d1 * d2;

    if (d >= ((4 + level) * MAX_UCHAR) / 80) {
        d = ((4 + level) * MAX_UCHAR - 1) / 80;
    }

    if ((4 + level) * a > 80 * d) {
        return true;
    }

    if (monrace.kind_flags.has_not(MonsterKindType::UNIQUE)) {
        return false;
    }

    if ((4 + level) * (2 * a) > 80 * d) {
        return true;
    }

    return false;
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
    MonsterRaceId r_idx = lore_ptr->r_idx;

    if (!know_details(r_idx) && !lore_ptr->know_everything) {
        // ダメージ量の情報なし
        lore_ptr->lore_msgs.emplace_back(format(msg, ""), color);
        return;
    }

    int base_damage = monspell_race_damage(player_ptr, ms_type, r_idx, BASE_DAM);
    int dice_num = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_NUM);
    int dice_side = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_SIDE);
    int dice_mult = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_MULT);
    int dice_div = monspell_race_damage(player_ptr, ms_type, r_idx, DICE_DIV);
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
