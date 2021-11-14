/*!
 * @file learnt-info.cpp
 * @brief 青魔法の情報表示処理定義
 */

#include "blue-magic/learnt-info.h"
#include "cmd-action/cmd-spell.h"
#include "lore/lore-calculator.h" //!< @todo 少し違和感.
#include "monster-race/race-ability-flags.h"
#include "mspell/mspell-damage-calculator.h"
#include "system/player-type-definition.h"

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
 * @param tmp 返すメッセージを格納する配列
 */
static void set_bluemage_damage(PlayerType *player_ptr, MonsterAbilityType ms_type, PLAYER_LEVEL plev, concptr msg, char *tmp)
{
    int base_damage = monspell_bluemage_damage(player_ptr, ms_type, plev, BASE_DAM);
    int dice_num = monspell_bluemage_damage(player_ptr, ms_type, plev, DICE_NUM);
    int dice_side = monspell_bluemage_damage(player_ptr, ms_type, plev, DICE_SIDE);
    int dice_mult = monspell_bluemage_damage(player_ptr, ms_type, plev, DICE_MULT);
    int dice_div = monspell_bluemage_damage(player_ptr, ms_type, plev, DICE_DIV);
    char dmg_str[80];
    dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div, dmg_str);
    sprintf(tmp, " %s %s", msg, dmg_str);
}

/*!
 * @brief 受け取ったモンスター魔法のIDに応じて青魔法の効果情報をまとめたフォーマットを返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param p 情報を返す文字列参照ポインタ
 * @param power モンスター魔法のID
 */
void learnt_info(PlayerType *player_ptr, char *p, MonsterAbilityType power)
{
    PLAYER_LEVEL plev = get_pseudo_monstetr_level(player_ptr);

    strcpy(p, "");

    switch (power) {
    case MonsterAbilityType::SHRIEK:
    case MonsterAbilityType::XXX1:
    case MonsterAbilityType::XXX2:
    case MonsterAbilityType::XXX3:
    case MonsterAbilityType::XXX4:
    case MonsterAbilityType::SCARE:
    case MonsterAbilityType::BLIND:
    case MonsterAbilityType::CONF:
    case MonsterAbilityType::SLOW:
    case MonsterAbilityType::HOLD:
    case MonsterAbilityType::HAND_DOOM:
    case MonsterAbilityType::WORLD:
    case MonsterAbilityType::SPECIAL:
    case MonsterAbilityType::TELE_TO:
    case MonsterAbilityType::TELE_AWAY:
    case MonsterAbilityType::TELE_LEVEL:
    case MonsterAbilityType::DARKNESS:
    case MonsterAbilityType::TRAPS:
    case MonsterAbilityType::FORGET:
    case MonsterAbilityType::S_KIN:
    case MonsterAbilityType::S_CYBER:
    case MonsterAbilityType::S_MONSTER:
    case MonsterAbilityType::S_MONSTERS:
    case MonsterAbilityType::S_ANT:
    case MonsterAbilityType::S_SPIDER:
    case MonsterAbilityType::S_HOUND:
    case MonsterAbilityType::S_HYDRA:
    case MonsterAbilityType::S_ANGEL:
    case MonsterAbilityType::S_DEMON:
    case MonsterAbilityType::S_UNDEAD:
    case MonsterAbilityType::S_DRAGON:
    case MonsterAbilityType::S_HI_UNDEAD:
    case MonsterAbilityType::S_HI_DRAGON:
    case MonsterAbilityType::S_AMBERITES:
    case MonsterAbilityType::S_UNIQUE:
        break;
    case MonsterAbilityType::BA_MANA:
    case MonsterAbilityType::BA_DARK:
    case MonsterAbilityType::BA_LITE:
        set_bluemage_damage(player_ptr, power, plev, KWD_DAM, p);
        break;
    case MonsterAbilityType::DISPEL:
        break;
    case MonsterAbilityType::ROCKET:
    case MonsterAbilityType::SHOOT:
    case MonsterAbilityType::BR_ACID:
    case MonsterAbilityType::BR_ELEC:
    case MonsterAbilityType::BR_FIRE:
    case MonsterAbilityType::BR_COLD:
    case MonsterAbilityType::BR_POIS:
    case MonsterAbilityType::BR_NUKE:
    case MonsterAbilityType::BR_NEXU:
    case MonsterAbilityType::BR_TIME:
    case MonsterAbilityType::BR_GRAV:
    case MonsterAbilityType::BR_MANA:
    case MonsterAbilityType::BR_NETH:
    case MonsterAbilityType::BR_LITE:
    case MonsterAbilityType::BR_DARK:
    case MonsterAbilityType::BR_CONF:
    case MonsterAbilityType::BR_SOUN:
    case MonsterAbilityType::BR_CHAO:
    case MonsterAbilityType::BR_DISE:
    case MonsterAbilityType::BR_SHAR:
    case MonsterAbilityType::BR_PLAS:
    case MonsterAbilityType::BR_INER:
    case MonsterAbilityType::BR_FORC:
    case MonsterAbilityType::BR_DISI:
    case MonsterAbilityType::BA_NUKE:
    case MonsterAbilityType::BA_CHAO:
    case MonsterAbilityType::BA_ACID:
    case MonsterAbilityType::BA_ELEC:
    case MonsterAbilityType::BA_FIRE:
    case MonsterAbilityType::BA_COLD:
    case MonsterAbilityType::BA_POIS:
    case MonsterAbilityType::BA_NETH:
    case MonsterAbilityType::BA_WATE:
        set_bluemage_damage(player_ptr, power, plev, KWD_DAM, p);
        break;
    case MonsterAbilityType::DRAIN_MANA:
        set_bluemage_damage(player_ptr, power, plev, KWD_HEAL, p);
        break;
    case MonsterAbilityType::MIND_BLAST:
    case MonsterAbilityType::BRAIN_SMASH:
    case MonsterAbilityType::CAUSE_1:
    case MonsterAbilityType::CAUSE_2:
    case MonsterAbilityType::CAUSE_3:
    case MonsterAbilityType::CAUSE_4:
    case MonsterAbilityType::BO_ACID:
    case MonsterAbilityType::BO_ELEC:
    case MonsterAbilityType::BO_FIRE:
    case MonsterAbilityType::BO_COLD:
    case MonsterAbilityType::BO_NETH:
    case MonsterAbilityType::BO_WATE:
    case MonsterAbilityType::BO_MANA:
    case MonsterAbilityType::BO_PLAS:
    case MonsterAbilityType::BO_ICEE:
    case MonsterAbilityType::MISSILE:
        set_bluemage_damage(player_ptr, power, plev, KWD_DAM, p);
        break;
    case MonsterAbilityType::HASTE:
        sprintf(p, " %sd%d+%d", KWD_DURATION, 20 + plev, plev);
        break;
    case MonsterAbilityType::HEAL:
        set_bluemage_damage(player_ptr, power, plev, KWD_HEAL, p);
        break;
    case MonsterAbilityType::INVULNER:
        sprintf(p, " %sd7+7", KWD_DURATION);
        break;
    case MonsterAbilityType::BLINK:
        sprintf(p, " %s10", KWD_SPHERE);
        break;
    case MonsterAbilityType::TPORT:
        sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
        break;
    case MonsterAbilityType::PSY_SPEAR:
        set_bluemage_damage(player_ptr, power, plev, KWD_DAM, p);
        break;
        break;
    case MonsterAbilityType::RAISE_DEAD:
        sprintf(p, " %s5", KWD_SPHERE);
        break;
    default:
        break;
    }
}
