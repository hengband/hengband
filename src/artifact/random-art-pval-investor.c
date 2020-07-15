/*!
 * @brief ランダムアーティファクトにpvalを追加する処理
 * @date 2020/07/14
 * @author Hourier
 */

#include "artifact/random-art-pval-investor.h"
#include "artifact/random-art-bias-types.h"
#include "object-hook/hook-weapon.h"
#include "object-enchant/tr-types.h"
#include "sv-definition/sv-armor-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

static bool bias_warrior_strength(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_STR))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_STR);
    return one_in_(2);
}

static bool bias_warrior_constitution(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_CON))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_CON);
    return one_in_(2);
}

static bool bias_warrior_dexterity(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_DEX))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_DEX);
    return one_in_(2);
}

static bool bias_mage_intelligence(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_INT))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_INT);
    return one_in_(2);
}

static bool bias_mage_mastery(object_type *o_ptr)
{
    if ((o_ptr->tval != TV_GLOVES) || have_flag(o_ptr->art_flags, TR_MAGIC_MASTERY))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_MAGIC_MASTERY);
    return one_in_(2);
}

static bool bias_priest_wisdom(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_WIS))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_WIS);
    return one_in_(2);
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにpval能力を付加する。/ Add one pval on generation of randam artifact.
 * @details 優先的に付加されるpvalがランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は腕力、知力、賢さ、器用さ、耐久、魅力、探索、隠密、赤外線視力、加速。武器のみ採掘、追加攻撃も候補に入る。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void random_plus(object_type *o_ptr)
{
    int this_type = object_is_weapon_ammo(o_ptr) ? 23 : 19;
    switch (o_ptr->artifact_bias) {
    case BIAS_WARRIOR:
        if (bias_warrior_strength(o_ptr) || bias_warrior_constitution(o_ptr) || bias_warrior_dexterity(o_ptr))
            return;

        break;
    case BIAS_MAGE:
        if (bias_mage_intelligence(o_ptr) || bias_mage_mastery(o_ptr))
            return;

        break;
    case BIAS_PRIESTLY:
        if (bias_priest_wisdom(o_ptr))
            return;

        break;
    case BIAS_RANGER:
        if (!(have_flag(o_ptr->art_flags, TR_DEX))) {
            add_flag(o_ptr->art_flags, TR_DEX);
            if (one_in_(2))
                return;
        }

        if (!(have_flag(o_ptr->art_flags, TR_CON))) {
            add_flag(o_ptr->art_flags, TR_CON);
            if (one_in_(2))
                return;
        }

        if (!(have_flag(o_ptr->art_flags, TR_STR))) {
            add_flag(o_ptr->art_flags, TR_STR);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_ROGUE:
        if (!(have_flag(o_ptr->art_flags, TR_STEALTH))) {
            add_flag(o_ptr->art_flags, TR_STEALTH);
            if (one_in_(2))
                return;
        }
        if (!(have_flag(o_ptr->art_flags, TR_SEARCH))) {
            add_flag(o_ptr->art_flags, TR_SEARCH);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_STR:
        if (!(have_flag(o_ptr->art_flags, TR_STR))) {
            add_flag(o_ptr->art_flags, TR_STR);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_WIS:
        if (!(have_flag(o_ptr->art_flags, TR_WIS))) {
            add_flag(o_ptr->art_flags, TR_WIS);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_INT:
        if (!(have_flag(o_ptr->art_flags, TR_INT))) {
            add_flag(o_ptr->art_flags, TR_INT);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_DEX:
        if (!(have_flag(o_ptr->art_flags, TR_DEX))) {
            add_flag(o_ptr->art_flags, TR_DEX);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_CON:
        if (!(have_flag(o_ptr->art_flags, TR_CON))) {
            add_flag(o_ptr->art_flags, TR_CON);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_CHR:
        if (!(have_flag(o_ptr->art_flags, TR_CHR))) {
            add_flag(o_ptr->art_flags, TR_CHR);
            if (one_in_(2))
                return;
        }

        break;
    }

    if ((o_ptr->artifact_bias == BIAS_MAGE || o_ptr->artifact_bias == BIAS_PRIESTLY) && (o_ptr->tval == TV_SOFT_ARMOR) && (o_ptr->sval == SV_ROBE)) {
        if (!(have_flag(o_ptr->art_flags, TR_DEC_MANA)) && one_in_(3)) {
            add_flag(o_ptr->art_flags, TR_DEC_MANA);
            if (one_in_(2))
                return;
        }
    }

    switch (randint1(this_type)) {
    case 1:
    case 2:
        add_flag(o_ptr->art_flags, TR_STR);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_STR;
        else if (!o_ptr->artifact_bias && one_in_(7))
            o_ptr->artifact_bias = BIAS_WARRIOR;
        break;
    case 3:
    case 4:
        add_flag(o_ptr->art_flags, TR_INT);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_INT;
        else if (!o_ptr->artifact_bias && one_in_(7))
            o_ptr->artifact_bias = BIAS_MAGE;
        break;
    case 5:
    case 6:
        add_flag(o_ptr->art_flags, TR_WIS);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_WIS;
        else if (!o_ptr->artifact_bias && one_in_(7))
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case 7:
    case 8:
        add_flag(o_ptr->art_flags, TR_DEX);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_DEX;
        else if (!o_ptr->artifact_bias && one_in_(7))
            o_ptr->artifact_bias = BIAS_ROGUE;
        break;
    case 9:
    case 10:
        add_flag(o_ptr->art_flags, TR_CON);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_CON;
        else if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_RANGER;
        break;
    case 11:
    case 12:
        add_flag(o_ptr->art_flags, TR_CHR);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_CHR;
        break;
    case 13:
    case 14:
        add_flag(o_ptr->art_flags, TR_STEALTH);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_ROGUE;
        break;
    case 15:
    case 16:
        add_flag(o_ptr->art_flags, TR_SEARCH);
        if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_RANGER;
        break;
    case 17:
    case 18:
        add_flag(o_ptr->art_flags, TR_INFRA);
        break;
    case 19:
        add_flag(o_ptr->art_flags, TR_SPEED);
        if (!o_ptr->artifact_bias && one_in_(11))
            o_ptr->artifact_bias = BIAS_ROGUE;
        break;
    case 20:
    case 21:
        add_flag(o_ptr->art_flags, TR_TUNNEL);
        break;
    case 22:
    case 23:
        if (o_ptr->tval == TV_BOW)
            random_plus(o_ptr);
        else {
            add_flag(o_ptr->art_flags, TR_BLOWS);
            if (!o_ptr->artifact_bias && one_in_(11))
                o_ptr->artifact_bias = BIAS_WARRIOR;
        }

        break;
    }
}
