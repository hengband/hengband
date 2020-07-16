#include "artifact/random-art-slay.h"
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

        if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_RANGER;

        return TRUE;
    default:
        add_flag(o_ptr->art_flags, TR_XTRA_SHOTS);
        if (!one_in_(7))
            remove_flag(o_ptr->art_flags, TR_XTRA_MIGHT);

        if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_RANGER;

        return TRUE;
    }
}

static bool random_art_slay_chaos(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_CHAOTIC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_CHAOTIC);
    return one_in_(2);
}

static bool random_art_slay_vampiric(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_VAMPIRIC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_VAMPIRIC);
    return one_in_(2);
}

static bool random_art_slay_brand_pois(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_BRAND_POIS) || one_in_(2))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_BRAND_POIS);
    return one_in_(2);
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
    if (random_art_slay_bow(o_ptr))
        return;

    switch (o_ptr->artifact_bias) {
    case BIAS_CHAOS:
        random_art_slay_chaos(o_ptr);
        break;
    case BIAS_PRIESTLY:
        if (((o_ptr->tval == TV_SWORD) || (o_ptr->tval == TV_POLEARM)) && !have_flag(o_ptr->art_flags, TR_BLESSED))
            add_flag(o_ptr->art_flags, TR_BLESSED);

        break;

    case BIAS_NECROMANTIC:
        if (random_art_slay_vampiric(o_ptr) || random_art_slay_brand_pois(o_ptr))
            return;

        break;
    case BIAS_RANGER:
        if (!(have_flag(o_ptr->art_flags, TR_SLAY_ANIMAL))) {
            add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_ROGUE:
        if ((((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_DAGGER)) || ((o_ptr->tval == TV_POLEARM) && (o_ptr->sval == SV_SPEAR)))
            && !(have_flag(o_ptr->art_flags, TR_THROW))) {
            /* Free power for rogues... */
            add_flag(o_ptr->art_flags, TR_THROW);
        }

        if (!(have_flag(o_ptr->art_flags, TR_BRAND_POIS))) {
            add_flag(o_ptr->art_flags, TR_BRAND_POIS);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_POIS:
        if (!(have_flag(o_ptr->art_flags, TR_BRAND_POIS))) {
            add_flag(o_ptr->art_flags, TR_BRAND_POIS);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_FIRE:
        if (!(have_flag(o_ptr->art_flags, TR_BRAND_FIRE))) {
            add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_COLD:
        if (!(have_flag(o_ptr->art_flags, TR_BRAND_COLD))) {
            add_flag(o_ptr->art_flags, TR_BRAND_COLD);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_ELEC:
        if (!(have_flag(o_ptr->art_flags, TR_BRAND_ELEC))) {
            add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_ACID:
        if (!(have_flag(o_ptr->art_flags, TR_BRAND_ACID))) {
            add_flag(o_ptr->art_flags, TR_BRAND_ACID);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_LAW:
        if (!(have_flag(o_ptr->art_flags, TR_SLAY_EVIL))) {
            add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
            if (one_in_(2))
                return;
        }

        if (!(have_flag(o_ptr->art_flags, TR_SLAY_UNDEAD))) {
            add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
            if (one_in_(2))
                return;
        }

        if (!(have_flag(o_ptr->art_flags, TR_SLAY_DEMON))) {
            add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
            if (one_in_(2))
                return;
        }

        break;
    }

    switch (randint1(36)) {
    case 1:
    case 2:
        if (one_in_(4)) {
            add_flag(o_ptr->art_flags, TR_KILL_ANIMAL);
        } else {
            add_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
        }
        break;
    case 3:
    case 4:
        if (one_in_(8)) {
            add_flag(o_ptr->art_flags, TR_KILL_EVIL);
        } else {
            add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
        }
        if (!o_ptr->artifact_bias && one_in_(2))
            o_ptr->artifact_bias = BIAS_LAW;
        else if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case 5:
    case 6:
        if (one_in_(4)) {
            add_flag(o_ptr->art_flags, TR_KILL_UNDEAD);
        } else {
            add_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
        }
        if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case 7:
    case 8:
        if (one_in_(4)) {
            add_flag(o_ptr->art_flags, TR_KILL_DEMON);
        } else {
            add_flag(o_ptr->art_flags, TR_SLAY_DEMON);
        }
        if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case 9:
    case 10:
        if (one_in_(4)) {
            add_flag(o_ptr->art_flags, TR_KILL_ORC);
        } else {
            add_flag(o_ptr->art_flags, TR_SLAY_ORC);
        }
        break;
    case 11:
    case 12:
        if (one_in_(4)) {
            add_flag(o_ptr->art_flags, TR_KILL_TROLL);
        } else {
            add_flag(o_ptr->art_flags, TR_SLAY_TROLL);
        }
        break;
    case 13:
    case 14:
        if (one_in_(4)) {
            add_flag(o_ptr->art_flags, TR_KILL_GIANT);
        } else {
            add_flag(o_ptr->art_flags, TR_SLAY_GIANT);
        }

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
        if (o_ptr->tval == TV_SWORD) {
            add_flag(o_ptr->art_flags, TR_VORPAL);
            if (!o_ptr->artifact_bias && one_in_(9))
                o_ptr->artifact_bias = BIAS_WARRIOR;
        } else
            random_slay(o_ptr);
        break;
    case 20:
        add_flag(o_ptr->art_flags, TR_IMPACT);
        break;
    case 21:
    case 22:
        add_flag(o_ptr->art_flags, TR_BRAND_FIRE);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_FIRE;
        break;
    case 23:
    case 24:
        add_flag(o_ptr->art_flags, TR_BRAND_COLD);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_COLD;
        break;
    case 25:
    case 26:
        add_flag(o_ptr->art_flags, TR_BRAND_ELEC);
        if (!o_ptr->artifact_bias)
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
        if (!o_ptr->artifact_bias && !one_in_(3))
            o_ptr->artifact_bias = BIAS_POIS;
        else if (!o_ptr->artifact_bias && one_in_(6))
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
        else if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_ROGUE;
        break;
    case 31:
        add_flag(o_ptr->art_flags, TR_VAMPIRIC);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
        break;
    case 32:
        add_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = (one_in_(2) ? BIAS_MAGE : BIAS_PRIESTLY);
        break;
    case 33:
    case 34:
        if (one_in_(4)) {
            add_flag(o_ptr->art_flags, TR_KILL_HUMAN);
        } else {
            add_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
        }

        break;
    default:
        add_flag(o_ptr->art_flags, TR_CHAOTIC);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_CHAOS;
        break;
    }
}
