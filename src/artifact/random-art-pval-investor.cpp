/*!
 * @file random-art-pval-investor.cpp
 * @brief ランダムアーティファクトにpvalを追加する処理
 * @date 2020/07/14
 * @author Hourier
 */

#include "artifact/random-art-pval-investor.h"
#include "artifact/random-art-bias-types.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-weapon.h"
#include "sv-definition/sv-armor-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

static bool random_art_bias_strength(object_type *o_ptr)
{
    if (o_ptr->art_flags.has(TR_STR))
        return false;

    o_ptr->art_flags.set(TR_STR);
    return one_in_(2);
}

static bool random_art_bias_intelligence(object_type *o_ptr)
{
    if (o_ptr->art_flags.has(TR_INT))
        return false;

    o_ptr->art_flags.set(TR_INT);
    return one_in_(2);
}

static bool random_art_bias_wisdom(object_type *o_ptr)
{
    if (o_ptr->art_flags.has(TR_WIS))
        return false;

    o_ptr->art_flags.set(TR_WIS);
    return one_in_(2);
}

static bool random_art_bias_dexterity(object_type *o_ptr)
{
    if (o_ptr->art_flags.has(TR_DEX))
        return false;

    o_ptr->art_flags.set(TR_DEX);
    return one_in_(2);
}

static bool random_art_bias_constitution(object_type *o_ptr)
{
    if (o_ptr->art_flags.has(TR_CON))
        return false;

    o_ptr->art_flags.set(TR_CON);
    return one_in_(2);
}

static bool random_art_bias_charisma(object_type *o_ptr)
{
    if (o_ptr->art_flags.has(TR_CHR))
        return false;

    o_ptr->art_flags.set(TR_CHR);
    return one_in_(2);
}

static bool random_art_bias_magic_mastery(object_type *o_ptr)
{
    if ((o_ptr->tval != ItemKindType::GLOVES) || o_ptr->art_flags.has(TR_MAGIC_MASTERY))
        return false;

    o_ptr->art_flags.set(TR_MAGIC_MASTERY);
    return one_in_(2);
}

static bool random_art_bias_stealth(object_type *o_ptr)
{
    if (o_ptr->art_flags.has(TR_STEALTH))
        return false;

    o_ptr->art_flags.set(TR_STEALTH);
    return one_in_(2);
}

static bool random_art_bias_search(object_type *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SEARCH))
        return false;

    o_ptr->art_flags.set(TR_SEARCH);
    return one_in_(2);
}

static bool switch_random_art_bias(object_type *o_ptr)
{
    switch (o_ptr->artifact_bias) {
    case BIAS_WARRIOR:
        return random_art_bias_strength(o_ptr) || random_art_bias_constitution(o_ptr) || random_art_bias_dexterity(o_ptr);
    case BIAS_MAGE:
        return random_art_bias_intelligence(o_ptr) || random_art_bias_magic_mastery(o_ptr);
    case BIAS_RANGER:
        return random_art_bias_dexterity(o_ptr) || random_art_bias_constitution(o_ptr) || random_art_bias_strength(o_ptr);
    case BIAS_ROGUE:
        return random_art_bias_stealth(o_ptr) || random_art_bias_search(o_ptr);
    case BIAS_STR:
        return random_art_bias_strength(o_ptr);
    case BIAS_INT:
        return random_art_bias_intelligence(o_ptr);
    case BIAS_PRIESTLY:
    case BIAS_WIS:
        return random_art_bias_wisdom(o_ptr);
    case BIAS_DEX:
        return random_art_bias_dexterity(o_ptr);
    case BIAS_CON:
        return random_art_bias_constitution(o_ptr);
    case BIAS_CHR:
        return random_art_bias_charisma(o_ptr);
    default:
        return false;
    }
}

static bool random_art_bias_decrease_mana(object_type *o_ptr)
{
    if (((o_ptr->artifact_bias != BIAS_MAGE) && (o_ptr->artifact_bias != BIAS_PRIESTLY)) || (o_ptr->tval != ItemKindType::SOFT_ARMOR) || (o_ptr->sval != SV_ROBE)
        || o_ptr->art_flags.has(TR_DEC_MANA) || !one_in_(3))
        return false;

    o_ptr->art_flags.set(TR_DEC_MANA);
    return one_in_(2);
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにpval能力を付加する。/ Add one pval on generation of randam artifact.
 * @details 優先的に付加されるpvalがランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は腕力、知力、賢さ、器用さ、耐久、魅力、探索、隠密、赤外線視力、加速。武器のみ採掘、追加攻撃も候補に入る。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void random_plus(object_type *o_ptr)
{
    if (switch_random_art_bias(o_ptr) || random_art_bias_decrease_mana(o_ptr))
        return;

    int this_type = o_ptr->is_weapon_ammo() ? 23 : 19;
    switch (randint1(this_type)) {
    case 1:
    case 2:
        o_ptr->art_flags.set(TR_STR);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_STR;
        else if (!o_ptr->artifact_bias && one_in_(7))
            o_ptr->artifact_bias = BIAS_WARRIOR;

        break;
    case 3:
    case 4:
        o_ptr->art_flags.set(TR_INT);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_INT;
        else if (!o_ptr->artifact_bias && one_in_(7))
            o_ptr->artifact_bias = BIAS_MAGE;

        break;
    case 5:
    case 6:
        o_ptr->art_flags.set(TR_WIS);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_WIS;
        else if (!o_ptr->artifact_bias && one_in_(7))
            o_ptr->artifact_bias = BIAS_PRIESTLY;

        break;
    case 7:
    case 8:
        o_ptr->art_flags.set(TR_DEX);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_DEX;
        else if (!o_ptr->artifact_bias && one_in_(7))
            o_ptr->artifact_bias = BIAS_ROGUE;

        break;
    case 9:
    case 10:
        o_ptr->art_flags.set(TR_CON);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_CON;
        else if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_RANGER;

        break;
    case 11:
    case 12:
        o_ptr->art_flags.set(TR_CHR);
        if (!o_ptr->artifact_bias && !one_in_(13))
            o_ptr->artifact_bias = BIAS_CHR;

        break;
    case 13:
    case 14:
        o_ptr->art_flags.set(TR_STEALTH);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_ROGUE;

        break;
    case 15:
    case 16:
        o_ptr->art_flags.set(TR_SEARCH);
        if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_RANGER;

        break;
    case 17:
    case 18:
        o_ptr->art_flags.set(TR_INFRA);
        break;
    case 19:
        o_ptr->art_flags.set(TR_SPEED);
        if (!o_ptr->artifact_bias && one_in_(11))
            o_ptr->artifact_bias = BIAS_ROGUE;

        break;
    case 20:
    case 21:
        o_ptr->art_flags.set(TR_TUNNEL);
        break;
    case 22:
    case 23:
        if (o_ptr->tval == ItemKindType::BOW) {
            random_plus(o_ptr);
            break;
        }

        o_ptr->art_flags.set(TR_BLOWS);
        if (!o_ptr->artifact_bias && one_in_(11))
            o_ptr->artifact_bias = BIAS_WARRIOR;

        break;
    }
}
