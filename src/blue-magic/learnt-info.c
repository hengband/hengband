#include "blue-magic/learnt-info.h"
#include "cmd-action/cmd-spell.h"
#include "lore/lore-calculator.h" // todo 少し違和感.
#include "mspell/mspell-damage-calculator.h"
#include "mspell/mspell-type.h"

/*!
 * @brief モンスター魔法をプレイヤーが使用する場合の換算レベル
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param 換算レベル
 */
PLAYER_LEVEL get_pseudo_monstetr_level(player_type *caster_ptr)
{
    PLAYER_LEVEL monster_level = caster_ptr->lev + 40;
    return (monster_level * monster_level - 1550) / 130;
}

/*!
 * @brief 文字列に青魔導師の呪文の攻撃力を加える
 * @param SPELL_NUM 呪文番号
 * @param plev プレイヤーレベル
 * @param msg 表示する文字列
 * @param tmp 返すメッセージを格納する配列
 * @return なし
 */
static void set_bluemage_damage(player_type *learner_type, monster_spell_type ms_type, PLAYER_LEVEL plev, concptr msg, char *tmp)
{
    int base_damage = monspell_bluemage_damage(learner_type, ms_type, plev, BASE_DAM);
    int dice_num = monspell_bluemage_damage(learner_type, ms_type, plev, DICE_NUM);
    int dice_side = monspell_bluemage_damage(learner_type, ms_type, plev, DICE_SIDE);
    int dice_mult = monspell_bluemage_damage(learner_type, ms_type, plev, DICE_MULT);
    int dice_div = monspell_bluemage_damage(learner_type, ms_type, plev, DICE_DIV);
    char dmg_str[80];
    dice_to_string(base_damage, dice_num, dice_side, dice_mult, dice_div, dmg_str);
    sprintf(tmp, " %s %s", msg, dmg_str);
}

/*!
 * @brief 受け取ったモンスター魔法のIDに応じて青魔法の効果情報をまとめたフォーマットを返す
 * @param learner_ptr プレーヤーへの参照ポインタ
 * @param p 情報を返す文字列参照ポインタ
 * @param power モンスター魔法のID
 * @return なし
 */
void learnt_info(player_type *learner_ptr, char *p, int power)
{
    PLAYER_LEVEL plev = get_pseudo_monstetr_level(learner_ptr);

    strcpy(p, "");

    switch (power) {
    case MS_SHRIEK:
    case MS_XXX1:
    case MS_XXX2:
    case MS_XXX3:
    case MS_XXX4:
    case MS_SCARE:
    case MS_BLIND:
    case MS_CONF:
    case MS_SLOW:
    case MS_SLEEP:
    case MS_HAND_DOOM:
    case MS_WORLD:
    case MS_SPECIAL:
    case MS_TELE_TO:
    case MS_TELE_AWAY:
    case MS_TELE_LEVEL:
    case MS_DARKNESS:
    case MS_MAKE_TRAP:
    case MS_FORGET:
    case MS_S_KIN:
    case MS_S_CYBER:
    case MS_S_MONSTER:
    case MS_S_MONSTERS:
    case MS_S_ANT:
    case MS_S_SPIDER:
    case MS_S_HOUND:
    case MS_S_HYDRA:
    case MS_S_ANGEL:
    case MS_S_DEMON:
    case MS_S_UNDEAD:
    case MS_S_DRAGON:
    case MS_S_HI_UNDEAD:
    case MS_S_HI_DRAGON:
    case MS_S_AMBERITE:
    case MS_S_UNIQUE:
        break;
    case MS_BALL_MANA:
    case MS_BALL_DARK:
    case MS_STARBURST:
        set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p);
        break;
    case MS_DISPEL:
        break;
    case MS_ROCKET:
    case MS_SHOOT:
    case MS_BR_ACID:
    case MS_BR_ELEC:
    case MS_BR_FIRE:
    case MS_BR_COLD:
    case MS_BR_POIS:
    case MS_BR_NUKE:
    case MS_BR_NEXUS:
    case MS_BR_TIME:
    case MS_BR_GRAVITY:
    case MS_BR_MANA:
    case MS_BR_NETHER:
    case MS_BR_LITE:
    case MS_BR_DARK:
    case MS_BR_CONF:
    case MS_BR_SOUND:
    case MS_BR_CHAOS:
    case MS_BR_DISEN:
    case MS_BR_SHARDS:
    case MS_BR_PLASMA:
    case MS_BR_INERTIA:
    case MS_BR_FORCE:
    case MS_BR_DISI:
    case MS_BALL_NUKE:
    case MS_BALL_CHAOS:
    case MS_BALL_ACID:
    case MS_BALL_ELEC:
    case MS_BALL_FIRE:
    case MS_BALL_COLD:
    case MS_BALL_POIS:
    case MS_BALL_NETHER:
    case MS_BALL_WATER:
        set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p);
        break;
    case MS_DRAIN_MANA:
        set_bluemage_damage(learner_ptr, power, plev, KWD_HEAL, p);
        break;
    case MS_MIND_BLAST:
    case MS_BRAIN_SMASH:
    case MS_CAUSE_1:
    case MS_CAUSE_2:
    case MS_CAUSE_3:
    case MS_CAUSE_4:
    case MS_BOLT_ACID:
    case MS_BOLT_ELEC:
    case MS_BOLT_FIRE:
    case MS_BOLT_COLD:
    case MS_BOLT_NETHER:
    case MS_BOLT_WATER:
    case MS_BOLT_MANA:
    case MS_BOLT_PLASMA:
    case MS_BOLT_ICE:
    case MS_MAGIC_MISSILE:
        set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p);
        break;
    case MS_SPEED:
        sprintf(p, " %sd%d+%d", KWD_DURATION, 20 + plev, plev);
        break;
    case MS_HEAL:
        set_bluemage_damage(learner_ptr, power, plev, KWD_HEAL, p);
        break;
    case MS_INVULNER:
        sprintf(p, " %sd7+7", KWD_DURATION);
        break;
    case MS_BLINK:
        sprintf(p, " %s10", KWD_SPHERE);
        break;
    case MS_TELEPORT:
        sprintf(p, " %s%d", KWD_SPHERE, plev * 5);
        break;
    case MS_PSY_SPEAR:
        set_bluemage_damage(learner_ptr, power, plev, KWD_DAM, p);
        break;
        break;
    case MS_RAISE_DEAD:
        sprintf(p, " %s5", KWD_SPHERE);
        break;
    default:
        break;
    }
}
