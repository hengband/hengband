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
PLAYER_LEVEL get_pseudo_monstetr_level(player_type *player_ptr)
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
static void set_bluemage_damage(player_type *player_ptr, RF_ABILITY ms_type, PLAYER_LEVEL plev, concptr msg, char *tmp)
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
void learnt_info(player_type *player_ptr, char *p, RF_ABILITY power)
{
    PLAYER_LEVEL plev = get_pseudo_monstetr_level(player_ptr);

    strcpy(p, "");

    switch (power) {
    case RF_ABILITY::SHRIEK:
    case RF_ABILITY::XXX1:
    case RF_ABILITY::XXX2:
    case RF_ABILITY::XXX3:
    case RF_ABILITY::XXX4:
    case RF_ABILITY::SCARE:
    case RF_ABILITY::BLIND:
    case RF_ABILITY::CONF:
    case RF_ABILITY::SLOW:
    case RF_ABILITY::HOLD:
    case RF_ABILITY::HAND_DOOM:
    case RF_ABILITY::WORLD:
    case RF_ABILITY::SPECIAL:
    case RF_ABILITY::TELE_TO:
    case RF_ABILITY::TELE_AWAY:
    case RF_ABILITY::TELE_LEVEL:
    case RF_ABILITY::DARKNESS:
    case RF_ABILITY::TRAPS:
    case RF_ABILITY::FORGET:
    case RF_ABILITY::S_KIN:
    case RF_ABILITY::S_CYBER:
    case RF_ABILITY::S_MONSTER:
    case RF_ABILITY::S_MONSTERS:
    case RF_ABILITY::S_ANT:
    case RF_ABILITY::S_SPIDER:
    case RF_ABILITY::S_HOUND:
    case RF_ABILITY::S_HYDRA:
    case RF_ABILITY::S_ANGEL:
    case RF_ABILITY::S_DEMON:
    case RF_ABILITY::S_UNDEAD:
    case RF_ABILITY::S_DRAGON:
    case RF_ABILITY::S_HI_UNDEAD:
    case RF_ABILITY::S_HI_DRAGON:
    case RF_ABILITY::S_AMBERITES:
    case RF_ABILITY::S_UNIQUE:
        break;
    case RF_ABILITY::BA_MANA:
    case RF_ABILITY::BA_DARK:
    case RF_ABILITY::BA_LITE:
        set_bluemage_damage(player_ptr, power, plev, KWD_DAM, p);
        break;
    case RF_ABILITY::DISPEL:
        break;
    case RF_ABILITY::ROCKET:
    case RF_ABILITY::SHOOT:
    case RF_ABILITY::BR_ACID:
    case RF_ABILITY::BR_ELEC:
    case RF_ABILITY::BR_FIRE:
    case RF_ABILITY::BR_COLD:
    case RF_ABILITY::BR_POIS:
    case RF_ABILITY::BR_NUKE:
    case RF_ABILITY::BR_NEXU:
    case RF_ABILITY::BR_TIME:
    case RF_ABILITY::BR_GRAV:
    case RF_ABILITY::BR_MANA:
    case RF_ABILITY::BR_NETH:
    case RF_ABILITY::BR_LITE:
    case RF_ABILITY::BR_DARK:
    case RF_ABILITY::BR_CONF:
    case RF_ABILITY::BR_SOUN:
    case RF_ABILITY::BR_CHAO:
    case RF_ABILITY::BR_DISE:
    case RF_ABILITY::BR_SHAR:
    case RF_ABILITY::BR_PLAS:
    case RF_ABILITY::BR_INER:
    case RF_ABILITY::BR_FORC:
    case RF_ABILITY::BR_DISI:
    case RF_ABILITY::BA_NUKE:
    case RF_ABILITY::BA_CHAO:
    case RF_ABILITY::BA_ACID:
    case RF_ABILITY::BA_ELEC:
    case RF_ABILITY::BA_FIRE:
    case RF_ABILITY::BA_COLD:
    case RF_ABILITY::BA_POIS:
    case RF_ABILITY::BA_NETH:
    case RF_ABILITY::BA_WATE:
        set_bluemage_damage(player_ptr, power, plev, KWD_DAM, p);
        break;
    case RF_ABILITY::DRAIN_MANA:
        set_bluemage_damage(player_ptr, power, plev, KWD_HEAL, p);
        break;
    case RF_ABILITY::MIND_BLAST:
    case RF_ABILITY::BRAIN_SMASH:
    case RF_ABILITY::CAUSE_1:
    case RF_ABILITY::CAUSE_2:
    case RF_ABILITY::CAUSE_3:
    case RF_ABILITY::CAUSE_4:
    case RF_ABILITY::BO_ACID:
    case RF_ABILITY::BO_ELEC:
    case RF_ABILITY::BO_FIRE:
    case RF_ABILITY::BO_COLD:
    case RF_ABILITY::BO_NETH:
    case RF_ABILITY::BO_WATE:
    case RF_ABILITY::BO_MANA:
    case RF_ABILITY::BO_PLAS:
    case RF_ABILITY::BO_ICEE:
    case RF_ABILITY::MISSILE:
        set_bluemage_damage(player_ptr, power, plev, KWD_DAM, p);
        break;
    case RF_ABILITY::HASTE:
        sprintf(p, " %sd%d+%d", KWD_DURATION, 20 + plev, plev);
        break;
    case RF_ABILITY::HEAL:
        set_bluemage_damage(player_ptr, power, plev, KWD_HEAL, p);
        break;
    case RF_ABILITY::INVULNER:
        sprintf(p, " %sd7+7", KWD_DURATION);
        break;
    case RF_ABILITY::BLINK:
        sprintf(p, " %s10", KWD_SPHERE);
        break;
    case RF_ABILITY::TPORT:
        sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
        break;
    case RF_ABILITY::PSY_SPEAR:
        set_bluemage_damage(player_ptr, power, plev, KWD_DAM, p);
        break;
        break;
    case RF_ABILITY::RAISE_DEAD:
        sprintf(p, " %s5", KWD_SPHERE);
        break;
    default:
        break;
    }
}
