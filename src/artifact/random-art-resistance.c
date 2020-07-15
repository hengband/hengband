#include "artifact/random-art-resistance.h"
#include "artifact/random-art-bias-types.h"
#include "object-enchant/tr-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

static bool random_art_resistance_acid(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_RES_ACID))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_ACID);
    return one_in_(2);
}

static bool random_art_immunity_acid(object_type *o_ptr)
{
    if (!one_in_(BIAS_LUCK) || have_flag(o_ptr->art_flags, TR_IM_ACID))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_IM_ACID);
    if (one_in_(IM_LUCK))
        return one_in_(2);

    remove_flag(o_ptr->art_flags, TR_IM_ELEC);
    remove_flag(o_ptr->art_flags, TR_IM_COLD);
    remove_flag(o_ptr->art_flags, TR_IM_FIRE);
    return one_in_(2);
}

static bool random_art_resistance_elec(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_RES_ELEC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_ELEC);
    return one_in_(2);
}

static bool random_art_aura_elec(object_type *o_ptr)
{
    if ((o_ptr->tval < TV_CLOAK) || (o_ptr->tval > TV_HARD_ARMOR) || have_flag(o_ptr->art_flags, TR_SH_ELEC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SH_ELEC);
    return one_in_(2);
}

static bool random_art_immunity_elec(object_type *o_ptr)
{
    if (!one_in_(BIAS_LUCK) || have_flag(o_ptr->art_flags, TR_IM_ELEC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_IM_ELEC);
    if (one_in_(IM_LUCK))
        return one_in_(2);

    remove_flag(o_ptr->art_flags, TR_IM_ACID);
    remove_flag(o_ptr->art_flags, TR_IM_COLD);
    remove_flag(o_ptr->art_flags, TR_IM_FIRE);
    return one_in_(2);
}

static bool random_art_resistance_fire(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_RES_FIRE))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_FIRE);
    return one_in_(2);
}

static bool random_art_aura_fire(object_type *o_ptr)
{
    if ((o_ptr->tval < TV_CLOAK) || (o_ptr->tval > TV_HARD_ARMOR) || have_flag(o_ptr->art_flags, TR_SH_FIRE))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SH_FIRE);
    return one_in_(2);
}

static bool random_art_immunity_fire(object_type *o_ptr)
{
    if (!one_in_(BIAS_LUCK) || have_flag(o_ptr->art_flags, TR_IM_FIRE))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_IM_FIRE);
    if (one_in_(IM_LUCK))
        return one_in_(2);

    remove_flag(o_ptr->art_flags, TR_IM_ELEC);
    remove_flag(o_ptr->art_flags, TR_IM_COLD);
    remove_flag(o_ptr->art_flags, TR_IM_ACID);
    return one_in_(2);
}

static bool random_art_resistance_cold(object_type *o_ptr)
{
    if (have_flag(o_ptr->art_flags, TR_RES_COLD))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_COLD);
    return one_in_(2);
}

static bool random_art_aura_cold(object_type *o_ptr)
{
    if ((o_ptr->tval < TV_CLOAK) || (o_ptr->tval > TV_HARD_ARMOR) || have_flag(o_ptr->art_flags, TR_SH_COLD))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SH_COLD);
    return one_in_(2);
}

static bool random_art_immunity_cold(object_type *o_ptr)
{
    if (!one_in_(BIAS_LUCK) || have_flag(o_ptr->art_flags, TR_IM_COLD))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_IM_COLD);
    if (one_in_(IM_LUCK))
        return one_in_(2);

    remove_flag(o_ptr->art_flags, TR_IM_ELEC);
    remove_flag(o_ptr->art_flags, TR_IM_COLD);
    remove_flag(o_ptr->art_flags, TR_IM_ACID);
    return one_in_(2);
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトに耐性を付加する。/ Add one resistance on generation of randam artifact.
 * @details 優先的に付加される耐性がランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は火炎、冷気、電撃、酸（以上免疫の可能性もあり）、
 * 毒、閃光、暗黒、破片、轟音、盲目、混乱、地獄、カオス、劣化、恐怖、火オーラ、冷気オーラ、電撃オーラ、反射。
 * 戦士系バイアスのみ反魔もつく。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void random_resistance(object_type *o_ptr)
{
    switch (o_ptr->artifact_bias) {
    case BIAS_ACID:
        if (random_art_resistance_acid(o_ptr) || random_art_immunity_acid(o_ptr))
            return;

        break;
    case BIAS_ELEC:
        if (random_art_resistance_elec(o_ptr) || random_art_aura_elec(o_ptr) || random_art_immunity_elec(o_ptr))
            return;

        break;

    case BIAS_FIRE:
        if (random_art_resistance_fire(o_ptr) || random_art_aura_fire(o_ptr) || random_art_immunity_fire(o_ptr))
            return;

        break;
    case BIAS_COLD:
        if (random_art_resistance_cold(o_ptr) || random_art_aura_cold(o_ptr) || random_art_immunity_cold(o_ptr))
            return;

        break;
    case BIAS_POIS:
        if (!(have_flag(o_ptr->art_flags, TR_RES_POIS))) {
            add_flag(o_ptr->art_flags, TR_RES_POIS);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_WARRIOR:
        if (!one_in_(3) && (!(have_flag(o_ptr->art_flags, TR_RES_FEAR)))) {
            add_flag(o_ptr->art_flags, TR_RES_FEAR);
            if (one_in_(2))
                return;
        }

        if (one_in_(3) && (!(have_flag(o_ptr->art_flags, TR_NO_MAGIC)))) {
            add_flag(o_ptr->art_flags, TR_NO_MAGIC);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_NECROMANTIC:
        if (!(have_flag(o_ptr->art_flags, TR_RES_NETHER))) {
            add_flag(o_ptr->art_flags, TR_RES_NETHER);
            if (one_in_(2))
                return;
        }

        if (!(have_flag(o_ptr->art_flags, TR_RES_POIS))) {
            add_flag(o_ptr->art_flags, TR_RES_POIS);
            if (one_in_(2))
                return;
        }

        if (!(have_flag(o_ptr->art_flags, TR_RES_DARK))) {
            add_flag(o_ptr->art_flags, TR_RES_DARK);
            if (one_in_(2))
                return;
        }

        break;

    case BIAS_CHAOS:
        if (!(have_flag(o_ptr->art_flags, TR_RES_CHAOS))) {
            add_flag(o_ptr->art_flags, TR_RES_CHAOS);
            if (one_in_(2))
                return;
        }

        if (!(have_flag(o_ptr->art_flags, TR_RES_CONF))) {
            add_flag(o_ptr->art_flags, TR_RES_CONF);
            if (one_in_(2))
                return;
        }

        if (!(have_flag(o_ptr->art_flags, TR_RES_DISEN))) {
            add_flag(o_ptr->art_flags, TR_RES_DISEN);
            if (one_in_(2))
                return;
        }

        break;
    }

    switch (randint1(42)) {
    case 1:
        if (!one_in_(WEIRD_LUCK))
            random_resistance(o_ptr);
        else {
            add_flag(o_ptr->art_flags, TR_IM_ACID);
            if (!o_ptr->artifact_bias)
                o_ptr->artifact_bias = BIAS_ACID;
        }

        break;
    case 2:
        if (!one_in_(WEIRD_LUCK))
            random_resistance(o_ptr);
        else {
            add_flag(o_ptr->art_flags, TR_IM_ELEC);
            if (!o_ptr->artifact_bias)
                o_ptr->artifact_bias = BIAS_ELEC;
        }

        break;
    case 3:
        if (!one_in_(WEIRD_LUCK))
            random_resistance(o_ptr);
        else {
            add_flag(o_ptr->art_flags, TR_IM_COLD);
            if (!o_ptr->artifact_bias)
                o_ptr->artifact_bias = BIAS_COLD;
        }

        break;
    case 4:
        if (!one_in_(WEIRD_LUCK))
            random_resistance(o_ptr);
        else {
            add_flag(o_ptr->art_flags, TR_IM_FIRE);
            if (!o_ptr->artifact_bias)
                o_ptr->artifact_bias = BIAS_FIRE;
        }

        break;
    case 5:
    case 6:
    case 13:
        add_flag(o_ptr->art_flags, TR_RES_ACID);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_ACID;
        break;
    case 7:
    case 8:
    case 14:
        add_flag(o_ptr->art_flags, TR_RES_ELEC);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_ELEC;
        break;
    case 9:
    case 10:
    case 15:
        add_flag(o_ptr->art_flags, TR_RES_FIRE);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_FIRE;
        break;
    case 11:
    case 12:
    case 16:
        add_flag(o_ptr->art_flags, TR_RES_COLD);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_COLD;
        break;
    case 17:
    case 18:
        add_flag(o_ptr->art_flags, TR_RES_POIS);
        if (!o_ptr->artifact_bias && !one_in_(4))
            o_ptr->artifact_bias = BIAS_POIS;
        else if (!o_ptr->artifact_bias && one_in_(2))
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
        else if (!o_ptr->artifact_bias && one_in_(2))
            o_ptr->artifact_bias = BIAS_ROGUE;
        break;
    case 19:
    case 20:
        add_flag(o_ptr->art_flags, TR_RES_FEAR);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_WARRIOR;
        break;
    case 21:
        add_flag(o_ptr->art_flags, TR_RES_LITE);
        break;
    case 22:
        add_flag(o_ptr->art_flags, TR_RES_DARK);
        break;
    case 23:
    case 24:
        add_flag(o_ptr->art_flags, TR_RES_BLIND);
        break;
    case 25:
    case 26:
        add_flag(o_ptr->art_flags, TR_RES_CONF);
        if (!o_ptr->artifact_bias && one_in_(6))
            o_ptr->artifact_bias = BIAS_CHAOS;
        break;
    case 27:
    case 28:
        add_flag(o_ptr->art_flags, TR_RES_SOUND);
        break;
    case 29:
    case 30:
        add_flag(o_ptr->art_flags, TR_RES_SHARDS);
        break;
    case 31:
    case 32:
        add_flag(o_ptr->art_flags, TR_RES_NETHER);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
        break;
    case 33:
    case 34:
        add_flag(o_ptr->art_flags, TR_RES_NEXUS);
        break;
    case 35:
    case 36:
        add_flag(o_ptr->art_flags, TR_RES_CHAOS);
        if (!o_ptr->artifact_bias && one_in_(2))
            o_ptr->artifact_bias = BIAS_CHAOS;
        break;
    case 37:
    case 38:
        add_flag(o_ptr->art_flags, TR_RES_DISEN);
        break;
    case 39:
        if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
            add_flag(o_ptr->art_flags, TR_SH_ELEC);
        else
            random_resistance(o_ptr);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_ELEC;
        break;
    case 40:
        if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
            add_flag(o_ptr->art_flags, TR_SH_FIRE);
        else
            random_resistance(o_ptr);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_FIRE;
        break;
    case 41:
        if (o_ptr->tval == TV_SHIELD || o_ptr->tval == TV_CLOAK || o_ptr->tval == TV_HELM || o_ptr->tval == TV_HARD_ARMOR)
            add_flag(o_ptr->art_flags, TR_REFLECT);
        else
            random_resistance(o_ptr);
        break;
    case 42:
        if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
            add_flag(o_ptr->art_flags, TR_SH_COLD);
        else
            random_resistance(o_ptr);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_COLD;
        break;
    }
}
