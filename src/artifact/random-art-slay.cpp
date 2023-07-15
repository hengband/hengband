/*!
 * @file random-art-slay.cpp
 * @brief ランダムアーティファクトのスレイ付加処理実装
 */

#include "artifact/random-art-slay.h"
#include "artifact/random-art-bias-types.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/item-entity.h"
#include "util/bit-flags-calculator.h"

static bool random_art_slay_bow(ItemEntity *o_ptr)
{
    if (o_ptr->bi_key.tval() != ItemKindType::BOW) {
        return false;
    }

    switch (randint1(6)) {
    case 1:
    case 2:
    case 3:
        o_ptr->art_flags.set(TR_XTRA_MIGHT);
        if (!one_in_(7)) {
            o_ptr->art_flags.reset(TR_XTRA_SHOTS);
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9)) {
            o_ptr->artifact_bias = BIAS_RANGER;
        }

        return true;
    default:
        o_ptr->art_flags.set(TR_XTRA_SHOTS);
        if (!one_in_(7)) {
            o_ptr->art_flags.reset(TR_XTRA_MIGHT);
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9)) {
            o_ptr->artifact_bias = BIAS_RANGER;
        }

        return true;
    }
}

static bool random_art_slay_chaos(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_CHAOTIC)) {
        return false;
    }

    o_ptr->art_flags.set(TR_CHAOTIC);
    return one_in_(2);
}

static bool random_art_brand_magical(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_BRAND_MAGIC)) {
        return false;
    }

    o_ptr->art_flags.set(TR_BRAND_MAGIC);
    return one_in_(3);
}

static bool random_art_slay_vampiric(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_VAMPIRIC)) {
        return false;
    }

    o_ptr->art_flags.set(TR_VAMPIRIC);
    return one_in_(2);
}

static bool random_art_slay_brand_acid(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_BRAND_ACID)) {
        return false;
    }

    o_ptr->art_flags.set(TR_BRAND_ACID);
    return one_in_(2);
}

static bool random_art_slay_brand_elec(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_BRAND_ELEC)) {
        return false;
    }

    o_ptr->art_flags.set(TR_BRAND_ELEC);
    return one_in_(2);
}

static bool random_art_slay_brand_fire(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_BRAND_FIRE)) {
        return false;
    }

    o_ptr->art_flags.set(TR_BRAND_FIRE);
    return one_in_(2);
}

static bool random_art_slay_brand_cold(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_BRAND_COLD)) {
        return false;
    }

    o_ptr->art_flags.set(TR_BRAND_COLD);
    return one_in_(2);
}

static bool random_art_slay_brand_pois(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_BRAND_POIS) || one_in_(2)) {
        return false;
    }

    o_ptr->art_flags.set(TR_BRAND_POIS);
    return one_in_(2);
}

static bool random_art_slay_animal(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SLAY_ANIMAL)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SLAY_ANIMAL);
    return one_in_(2);
}

static bool random_art_slay_evil(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SLAY_EVIL)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SLAY_EVIL);
    return one_in_(2);
}

static bool random_art_slay_undead(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SLAY_UNDEAD)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SLAY_UNDEAD);
    return one_in_(2);
}

static bool random_art_slay_demon(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SLAY_DEMON)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SLAY_DEMON);
    return one_in_(2);
}

static bool switch_random_art_slay(ItemEntity *o_ptr)
{
    switch (o_ptr->artifact_bias) {
    case BIAS_CHAOS:
        return random_art_slay_chaos(o_ptr);
    case BIAS_MAGE:
    case BIAS_INT:
        return random_art_brand_magical(o_ptr);
    case BIAS_PRIESTLY: {
        const auto tval = o_ptr->bi_key.tval();
        if (((tval == ItemKindType::SWORD) || (tval == ItemKindType::POLEARM)) && o_ptr->art_flags.has_not(TR_BLESSED)) {
            o_ptr->art_flags.set(TR_BLESSED);
        }

        return false;
    }
    case BIAS_NECROMANTIC:
        return random_art_slay_vampiric(o_ptr) || random_art_slay_brand_pois(o_ptr);
    case BIAS_RANGER:
        return random_art_slay_animal(o_ptr);
    case BIAS_ROGUE: {
        auto is_throwable = o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_DAGGER);
        is_throwable |= o_ptr->bi_key == BaseitemKey(ItemKindType::POLEARM, SV_SPEAR);
        if (is_throwable && o_ptr->art_flags.has_not(TR_THROW)) {
            o_ptr->art_flags.set(TR_THROW);
        }

        return random_art_slay_brand_pois(o_ptr);
    }
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
        return false;
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
 */
void random_slay(ItemEntity *o_ptr)
{
    if (random_art_slay_bow(o_ptr) || switch_random_art_slay(o_ptr)) {
        return;
    }

    switch (randint1(39)) {
    case 1:
    case 2:
        if (one_in_(4)) {
            o_ptr->art_flags.set(TR_KILL_ANIMAL);
        } else {
            o_ptr->art_flags.set(TR_SLAY_ANIMAL);
        }

        break;
    case 3:
    case 4:
        if (one_in_(8)) {
            o_ptr->art_flags.set(TR_KILL_EVIL);
        } else {
            o_ptr->art_flags.set(TR_SLAY_EVIL);
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(2)) {
            o_ptr->artifact_bias = BIAS_LAW;
            break;
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9)) {
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        }

        break;
    case 5:
    case 6:
        if (one_in_(4)) {
            o_ptr->art_flags.set(TR_KILL_UNDEAD);
        } else {
            o_ptr->art_flags.set(TR_SLAY_UNDEAD);
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9)) {
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        }

        break;
    case 7:
    case 8:
        if (one_in_(4)) {
            o_ptr->art_flags.set(TR_KILL_DEMON);
        } else {
            o_ptr->art_flags.set(TR_SLAY_DEMON);
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9)) {
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        }

        break;
    case 9:
    case 10:
        if (one_in_(4)) {
            o_ptr->art_flags.set(TR_KILL_ORC);
        } else {
            o_ptr->art_flags.set(TR_SLAY_ORC);
        }

        break;
    case 11:
    case 12:
        if (one_in_(4)) {
            o_ptr->art_flags.set(TR_KILL_TROLL);
        } else {
            o_ptr->art_flags.set(TR_SLAY_TROLL);
        }

        break;
    case 13:
    case 14:
        if (one_in_(4)) {
            o_ptr->art_flags.set(TR_KILL_GIANT);
        } else {
            o_ptr->art_flags.set(TR_SLAY_GIANT);
        }

        break;
    case 15:
    case 16:
        o_ptr->art_flags.set(TR_SLAY_DRAGON);
        break;
    case 17:
        o_ptr->art_flags.set(TR_KILL_DRAGON);
        break;
    case 18:
    case 19:
        if (o_ptr->bi_key.tval() != ItemKindType::SWORD) {
            random_slay(o_ptr);
            break;
        }

        o_ptr->art_flags.set(TR_VORPAL);
        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9)) {
            o_ptr->artifact_bias = BIAS_WARRIOR;
        }

        break;
    case 20:
        o_ptr->art_flags.set(TR_EARTHQUAKE);
        break;
    case 21:
    case 22:
        o_ptr->art_flags.set(TR_BRAND_FIRE);
        if (o_ptr->artifact_bias == BIAS_NONE) {
            o_ptr->artifact_bias = BIAS_FIRE;
        }

        break;
    case 23:
    case 24:
        o_ptr->art_flags.set(TR_BRAND_COLD);
        if (o_ptr->artifact_bias == BIAS_NONE) {
            o_ptr->artifact_bias = BIAS_COLD;
        }

        break;
    case 25:
    case 26:
        o_ptr->art_flags.set(TR_BRAND_ELEC);
        if (o_ptr->artifact_bias == BIAS_NONE) {
            o_ptr->artifact_bias = BIAS_ELEC;
        }

        break;
    case 27:
    case 28:
        o_ptr->art_flags.set(TR_BRAND_ACID);
        if (!o_ptr->artifact_bias) {
            o_ptr->artifact_bias = BIAS_ACID;
        }

        break;
    case 29:
    case 30:
        o_ptr->art_flags.set(TR_BRAND_POIS);
        if ((o_ptr->artifact_bias == BIAS_NONE) && !one_in_(3)) {
            o_ptr->artifact_bias = BIAS_POIS;
            break;
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(6)) {
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
            break;
        }

        if (o_ptr->artifact_bias == BIAS_NONE) {
            o_ptr->artifact_bias = BIAS_ROGUE;
        }

        break;
    case 31:
        o_ptr->art_flags.set(TR_VAMPIRIC);
        if (o_ptr->artifact_bias == BIAS_NONE) {
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
        }

        break;
    case 32:
        o_ptr->art_flags.set(TR_FORCE_WEAPON);
        if (o_ptr->artifact_bias == BIAS_NONE) {
            o_ptr->artifact_bias = (one_in_(2) ? BIAS_MAGE : BIAS_PRIESTLY);
        }

        break;
    case 33:
    case 34:
        if (one_in_(4)) {
            o_ptr->art_flags.set(TR_KILL_HUMAN);
        } else {
            o_ptr->art_flags.set(TR_SLAY_HUMAN);
        }

        break;
    case 35:
        o_ptr->art_flags.set(TR_BRAND_MAGIC);
        if (o_ptr->artifact_bias == BIAS_NONE) {
            o_ptr->artifact_bias = BIAS_MAGE;
        }
        break;
    case 36:
    case 37:
        o_ptr->art_flags.set(TR_CHAOTIC);
        if (o_ptr->artifact_bias == BIAS_NONE) {
            o_ptr->artifact_bias = BIAS_CHAOS;
        }

        break;
    default:
        if (one_in_(8)) {
            o_ptr->art_flags.set(TR_KILL_GOOD);
        } else {
            o_ptr->art_flags.set(TR_SLAY_GOOD);
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(2)) {
            o_ptr->artifact_bias = BIAS_POIS;
            break;
        }

        if ((o_ptr->artifact_bias == BIAS_NONE) && one_in_(9)) {
            o_ptr->artifact_bias = BIAS_ROGUE;
        }

        break;
    }
}
