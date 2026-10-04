/*!
 * @file learnt-info.cpp
 * @brief 青魔法の情報表示処理定義
 */

#include "blue-magic/learnt-info.h"
#include "cmd-action/cmd-spell.h"
#include "lore/lore-calculator.h" //!< @todo 少し違和感.
#include "monster-race/race-ability-flags.h"
#include "monster-race/race-ability-mask.h"
#include "mspell/mspell-damage-calculator.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"

/*!
 * @brief モンスター魔法をプレイヤーが使用する場合の換算レベル
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param 換算レベル
 */
PLAYER_LEVEL get_pseudo_monstetr_level(PlayerType *player_ptr)
{
    PLAYER_LEVEL monster_level = player_ptr->lev + 40;
    return (monster_level * monster_level - 1550) / 130;
}

/*!
 * @brief 文字列に青魔導師の呪文の攻撃力を加える
 * @param SPELL_NUM 呪文番号
 * @param plev プレイヤーレベル
 * @param msg 表示する文字列
 * @return std::string 潜在的な損傷の説明
 */
static std::string set_bluemage_damage(PlayerType *player_ptr, MonsterAbilityType ms_type, PLAYER_LEVEL plev, concptr msg)
{
    int base_damage = monspell_bluemage_damage(player_ptr, ms_type, plev, BASE_DAM);
    int dice_num = monspell_bluemage_damage(player_ptr, ms_type, plev, DICE_NUM);
    int dice_side = monspell_bluemage_damage(player_ptr, ms_type, plev, DICE_SIDE);
    int dice_mult = monspell_bluemage_damage(player_ptr, ms_type, plev, DICE_MULT);
    int dice_div = monspell_bluemage_damage(player_ptr, ms_type, plev, DICE_DIV);
    return format(" %s %s", msg, dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div).data());
}

/*!
 * @brief 受け取ったモンスター魔法のIDに応じて青魔法の効果情報をまとめたフォーマットを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param power モンスター魔法のID
 * @return std::string パワーについてのコメント
 */
std::string learnt_info(PlayerType *player_ptr, MonsterAbilityType power)
{
    PLAYER_LEVEL plev = get_pseudo_monstetr_level(player_ptr);

    if (RF_ABILITY_DAMAGE_MASK.has(power)) {
        return set_bluemage_damage(player_ptr, power, plev, KWD_DAM);
    }

    switch (power) {
    case MonsterAbilityType::DRAIN_MANA:
        return set_bluemage_damage(player_ptr, power, plev, KWD_HEAL);
    case MonsterAbilityType::HASTE:
        return format(" %sd%d+%d", KWD_DURATION, 20 + plev, plev);
    case MonsterAbilityType::HEAL:
        return set_bluemage_damage(player_ptr, power, plev, KWD_HEAL);
    case MonsterAbilityType::INVULNER:
        return format(" %sd7+7", KWD_DURATION);
    case MonsterAbilityType::BLINK:
        return format(" %s10", KWD_SPHERE);
    case MonsterAbilityType::TPORT:
        return format(" %s%d", KWD_SPHERE, plev * 5);
    case MonsterAbilityType::RAISE_DEAD:
        return format(" %s5", KWD_SPHERE);
    default:
        return std::string();
    }
}
