﻿#include "artifact/random-art-slay.h"
#include "artifact/random-art-bias-types.h"
#include "object-enchant/tr-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

static bool random_art_slay_bow(object_type *o_ptr)
{
    if (o_ptr->tval != TV_BOW)
        return FALSE;

    switch (randint1(6)) {
    case 1:
    case 2:
    case 3:
        add_flag(o_ptr->art_flags, TR_XTRA_MIGHT);
        if (!one_in_(7))
            remove_flag(o_ptr->art_flags, TR_XTRA_SHOTS);

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9))
            o_ptr->artifact_bias = BIAS_RANGER;

        return TRUE;
    default:
        add_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
        if (!one_in_(7))
            remove_flag(o_ptr->art_flags, TR_XTRA_MIGHT);

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9))
            o_ptr->artifact_bias = BIAS_RANGER;

        return TRUE;
    }
}

static bool random_art_slay_chaos(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_CHAOTIC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_CHAOTIC);
    return one_in_(2);
}

static bool random_art_slay_vampiric(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_VAMPIRIC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_VAMPIRIC);
    return one_in_(2);
}

static bool random_art_slay_brand_acid(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_BRAND_ACID))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_BRAND_ACID);
    return one_in_(2);
}

static bool random_art_slay_brand_elec(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_BRAND_ELEC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
    return one_in_(2);
}

static bool random_art_slay_brand_fire(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_BRAND_FIRE))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
    return one_in_(2);
}

static bool random_art_slay_brand_cold(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_BRAND_COLD))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_BRAND_COLD);
    return one_in_(2);
}

static bool random_art_slay_brand_pois(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_BRAND_POIS) || one_in_(2))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_BRAND_POIS);
    return one_in_(2);
}

static bool random_art_slay_animal(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SLAY_ANIMAL))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
    return one_in_(2);
}

static bool random_art_slay_evil(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SLAY_EVIL))
        return FALSE;
    
    add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
    return one_in_(2);
}

static bool random_art_slay_undead(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SLAY_UNDEAD))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
    return one_in_(2);
}

static bool random_art_slay_demon(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SLAY_DEMON))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
    return one_in_(2);
}

static bool switch_random_art_slay(object_type *o_ptr)
{
    switch (o_ptr->artifact_bias) {
    case BIAS_CHAOS:
        return random_art_slay_chaos(o_ptr);
    case BIAS_PRIESTLY:
        if (((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM)) && !has_flag(o_ptr->art_flags, TR_BLESSED))
            add_flag(o_ptr->art_flags, TR_BLESSED);

        return FALSE;
    case BIAS_NECROMANTIC:
        return random_art_slay_vampiric(o_ptr) || random_art_slay_brand_pois(o_ptr);
    case BIAS_RANGER:
        return random_art_slay_animal(o_ptr);
    case BIAS_ROGUE:
        if ((((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DAGGER)) || ((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_SPEAR)))
            && !(has_flag(o_ptr->art_flags, TR_THROW))) {
            add_flag(o_ptr->art_flags, TR_THROW);
        }

        return random_art_slay_brand_pois(o_ptr);
    case BIAS_POIS:
        return random_art_slay_brand_pois(o_ptr);
    case BIAS_ACID:
        return random_art_slay_brand_acid(o_ptr);
    case BIAS_ELEC:
        return random_art_slay_brand_elec(o_ptr);
    case BIAS_FIRE:
        return random_art_slay_brand_fire(o_ptr);
    case BIAS_COLD:
        return random_art_slay_brand_cold(o_ptr);
    case BIAS_LAW:
        return random_art_slay_evil(o_ptr) || random_art_slay_undead(o_ptr) || random_art_slay_demon(o_ptr);
    default:
        return FALSE;
    }
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにスレイ効果を付加する。/ Add one slaying on generation of randam artifact.
 * @details 優先的に付加される耐性がランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は強力射、高速射、混沌効果、吸血効果、祝福、投擲しやすい、焼棄、凍結、電撃、溶解、毒殺、
 * 動物スレイ、邪悪スレイ、悪魔スレイ、不死スレイ、オークスレイ、トロルスレイ、巨人スレイ、ドラゴンスレイ、
 * *ドラゴンスレイ*、人間スレイ、切れ味、地震、理力。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void random_slay(object_type *o_ptr)
{
    if (random_art_slay_bow(o_ptr) || switch_random_art_slay(o_ptr))
        return;

    switch (randint1(36)) {
    case 1:
    case 2:
        if (one_in_(4))
            add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
        else
            add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
        
        break;
    case 3:
    case 4:
        if (one_in_(8))
            add_flag(o_ptr->art_flags, TR_KILL_EVIL);
        else
            add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
        
        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(2)) {
            o_ptr->artifact_bias = BIAS_LAW;
            break;
        }
        
        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9))
            o_ptr->artifact_bias = BIAS_PRIESTLY;

        break;
    case 5:
    case 6:
        if (one_in_(4))
            add_flag(o_ptr->art_flags, TR_KILL_UNDEAD);
        else
            add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
        
        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9))
            o_ptr->artifact_bias = BIAS_PRIESTLY;

        break;
    case 7:
    case 8:
        if (one_in_(4))
            add_flag(o_ptr->art_flags, TR_KILL_DEMON);
        else
            add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
        
        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9))
            o_ptr->artifact_bias = BIAS_PRIESTLY;

        break;
    case 9:
    case 10:
        if (one_in_(4))
            add_flag(o_ptr->art_flags, TR_KILL_ORC);
        else
            add_flag(o_ptr->art_flags, TR_SLAY_ORC);
        
        break;
    case 11:
    case 12:
        if (one_in_(4))
            add_flag(o_ptr->art_flags, TR_KILL_TROLL);
        else
            add_flag(o_ptr->art_flags, TR_SLAY_TROLL);
        
        break;
    case 13:
    case 14:
        if (one_in_(4))
            add_flag(o_ptr->art_flags, TR_KILL_GIANT);
        else
            add_flag(o_ptr->art_flags, TR_SLAY_GIANT);
        
        break;
    case 15:
    case 16:
        add_flag(o_ptr->art_flags, TR_SLAY_DRAGON);
        break;
    case 17:
        add_flag(o_ptr->art_flags, TR_KILL_DRAGON);
        break;
    case 18:
    case 19:
        if (o_ptr->tval != TV_SWORD) {
            random_slay(o_ptr);
            break;
        }

        add_flag(o_ptr->art_flags, TR_VORPAL);
        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9))
            o_ptr->artifact_bias = BIAS_WARRIOR;

        break;
    case 20:
        add_flag(o_ptr->art_flags, TR_IMPACT);
        break;
    case 21:
    case 22:
        add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
        if (o_ptr->artifact_bias == BIAS_NONE)
            o_ptr->artifact_bias = BIAS_FIRE;

        break;
    case 23:
    case 24:
        add_flag(o_ptr->art_flags, TR_BRAND_COLD);
        if (o_ptr->artifact_bias == BIAS_NONE)
            o_ptr->artifact_bias = BIAS_COLD;

        break;
    case 25:
    case 26:
        add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
        if (o_ptr->artifact_bias == BIAS_NONE)
            o_ptr->artifact_bias = BIAS_ELEC;

        break;
    case 27:
    case 28:
        add_flag(o_ptr->art_flags, TR_BRAND_ACID);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_ACID;

        break;
    case 29:
    case 30:
        add_flag(o_ptr->art_flags, TR_BRAND_POIS);
        if ((o_ptr->artifact_bias == BIAS_NONE) && !one_in_(3)) {
            o_ptr->artifact_bias = BIAS_POIS;
            break;
        }
        
        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(6)) {
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
            break;
        }
        
        if (o_ptr->artifact_bias == BIAS_NONE)
            o_ptr->artifact_bias = BIAS_ROGUE;

        break;
    case 31:
        add_flag(o_ptr->art_flags, TR_VAMPIRIC);
        if (o_ptr->artifact_bias == BIAS_NONE)
            o_ptr->artifact_bias = BIAS_NECROMANTIC;

        break;
    case 32:
        add_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
        if (o_ptr->artifact_bias == BIAS_NONE)
            o_ptr->artifact_bias = (one_in_(2) ? BIAS_MAGE : BIAS_PRIESTLY);

        break;
    case 33:
    case 34:
        if (one_in_(4))
            add_flag(o_ptr->art_flags, TR_KILL_HUMAN);
        else
            add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
        
        break;
    default:
        add_flag(o_ptr->art_flags, TR_CHAOTIC);
        if (o_ptr->artifact_bias == BIAS_NONE)
            o_ptr->artifact_bias = BIAS_CHAOS;

        break;
    }
}
