#include "artifact/random-art-resistance.h"
#include "artifact/random-art-bias-types.h"
#include "object-enchant/tr-types.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

static bool random_art_resistance_acid(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_RES_ACID))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_ACID);
    return one_in_(2);
}

static bool random_art_immunity_acid(object_type *o_ptr)
{
    if (!one_in_(BIAS_LUCK) || has_flag(o_ptr->art_flags, TR_IM_ACID))
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
    if (has_flag(o_ptr->art_flags, TR_RES_ELEC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_ELEC);
    return one_in_(2);
}

static bool random_art_aura_elec(object_type *o_ptr)
{
    if ((o_ptr->tval < TV_CLOAK) || (o_ptr->tval > TV_HARD_ARMOR) || has_flag(o_ptr->art_flags, TR_SH_ELEC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SH_ELEC);
    return one_in_(2);
}

static bool random_art_immunity_elec(object_type *o_ptr)
{
    if (!one_in_(BIAS_LUCK) || has_flag(o_ptr->art_flags, TR_IM_ELEC))
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
    if (has_flag(o_ptr->art_flags, TR_RES_FIRE))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_FIRE);
    return one_in_(2);
}

static bool random_art_aura_fire(object_type *o_ptr)
{
    if ((o_ptr->tval < TV_CLOAK) || (o_ptr->tval > TV_HARD_ARMOR) || has_flag(o_ptr->art_flags, TR_SH_FIRE))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SH_FIRE);
    return one_in_(2);
}

static bool random_art_immunity_fire(object_type *o_ptr)
{
    if (!one_in_(BIAS_LUCK) || has_flag(o_ptr->art_flags, TR_IM_FIRE))
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
    if (has_flag(o_ptr->art_flags, TR_RES_COLD))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_COLD);
    return one_in_(2);
}

static bool random_art_aura_cold(object_type *o_ptr)
{
    if ((o_ptr->tval < TV_CLOAK) || (o_ptr->tval > TV_HARD_ARMOR) || has_flag(o_ptr->art_flags, TR_SH_COLD))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SH_COLD);
    return one_in_(2);
}

static bool random_art_immunity_cold(object_type *o_ptr)
{
    if (!one_in_(BIAS_LUCK) || has_flag(o_ptr->art_flags, TR_IM_COLD))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_IM_COLD);
    if (one_in_(IM_LUCK))
        return one_in_(2);

    remove_flag(o_ptr->art_flags, TR_IM_ELEC);
    remove_flag(o_ptr->art_flags, TR_IM_COLD);
    remove_flag(o_ptr->art_flags, TR_IM_ACID);
    return one_in_(2);
}

static bool random_art_resistance_pois(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_RES_POIS))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_POIS);
    return one_in_(2);
}

static bool random_art_resistance_fear(object_type *o_ptr)
{
    if (one_in_(3) || has_flag(o_ptr->art_flags, TR_RES_FEAR))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_FEAR);
    return one_in_(2);
}

static bool random_art_resistance_no_magic(object_type *o_ptr)
{
    if (!one_in_(3) || has_flag(o_ptr->art_flags, TR_NO_MAGIC))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_NO_MAGIC);
    return one_in_(2);
}

static bool random_art_resistance_nether(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_RES_NETHER))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_NETHER);
    return one_in_(2);
}

static bool random_art_resistance_dark(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_RES_DARK))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_DARK);
    return one_in_(2);
}

static bool random_art_resistance_chaos(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_RES_CHAOS))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_CHAOS);
    return one_in_(2);
}

static bool random_art_resistance_confusion(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_RES_CONF))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_CONF);
    return one_in_(2);
}

static bool random_art_resistance_disenchant(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_RES_DISEN))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_RES_DISEN);
    return one_in_(2);
}

static bool switch_random_art_resistance(object_type *o_ptr)
{
    switch (o_ptr->artifact_bias) {
    case BIAS_ACID:
        return random_art_resistance_acid(o_ptr) || random_art_immunity_acid(o_ptr);
    case BIAS_ELEC:
        return random_art_resistance_elec(o_ptr) || random_art_aura_elec(o_ptr) || random_art_immunity_elec(o_ptr);
    case BIAS_FIRE:
        return random_art_resistance_fire(o_ptr) || random_art_aura_fire(o_ptr) || random_art_immunity_fire(o_ptr);
    case BIAS_COLD:
        return random_art_resistance_cold(o_ptr) || random_art_aura_cold(o_ptr) || random_art_immunity_cold(o_ptr);
    case BIAS_POIS:
        return random_art_resistance_pois(o_ptr);
    case BIAS_WARRIOR:
        return random_art_resistance_fear(o_ptr) || random_art_resistance_no_magic(o_ptr);
    case BIAS_NECROMANTIC:
        return random_art_resistance_nether(o_ptr) || random_art_resistance_pois(o_ptr) || random_art_resistance_dark(o_ptr);
    case BIAS_CHAOS:
        return random_art_resistance_chaos(o_ptr) || random_art_resistance_confusion(o_ptr) || random_art_resistance_disenchant(o_ptr);
    default:
        return FALSE;
    }
}

/* 一定確率で再試行する */
static void set_weird_bias_acid(object_type *o_ptr)
{
    if (!one_in_(WEIRD_LUCK)) {
        random_resistance(o_ptr);
        return;
    }

    add_flag(o_ptr->art_flags, TR_IM_ACID);
    if (!o_ptr->artifact_bias)
        o_ptr->artifact_bias = BIAS_ACID;
}

/* 一定確率で再試行する */
static void set_weird_bias_elec(object_type *o_ptr)
{
    if (!one_in_(WEIRD_LUCK)) {
        random_resistance(o_ptr);
        return;
    }

    add_flag(o_ptr->art_flags, TR_IM_ELEC);
    if (!o_ptr->artifact_bias)
        o_ptr->artifact_bias = BIAS_ELEC;
}

/* 一定確率で再試行する */
static void set_weird_bias_cold(object_type *o_ptr)
{
    if (!one_in_(WEIRD_LUCK)) {
        random_resistance(o_ptr);
        return;
    }

    add_flag(o_ptr->art_flags, TR_IM_COLD);
    if (!o_ptr->artifact_bias)
        o_ptr->artifact_bias = BIAS_COLD;
}

/* 一定確率で再試行する */
static void set_weird_bias_fire(object_type *o_ptr)
{
    if (!one_in_(WEIRD_LUCK)) {
        random_resistance(o_ptr);
        return;
    }

    add_flag(o_ptr->art_flags, TR_IM_FIRE);
    if (!o_ptr->artifact_bias)
        o_ptr->artifact_bias = BIAS_FIRE;
}

static void set_bias_pois(object_type *o_ptr)
{
    add_flag(o_ptr->art_flags, TR_RES_POIS);
    if (!o_ptr->artifact_bias && !one_in_(4)) {
        o_ptr->artifact_bias = BIAS_POIS;
        return;
    }

    if (!o_ptr->artifact_bias && one_in_(2)) {
        o_ptr->artifact_bias = BIAS_NECROMANTIC;
        return;
    }

    if (!o_ptr->artifact_bias && one_in_(2))
        o_ptr->artifact_bias = BIAS_ROGUE;
}

/* 一定確率で再試行する */
static void set_weird_bias_aura_elec(object_type *o_ptr)
{
    if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
        add_flag(o_ptr->art_flags, TR_SH_ELEC);
    else
        random_resistance(o_ptr);

    if (!o_ptr->artifact_bias)
        o_ptr->artifact_bias = BIAS_ELEC;
}

/* 一定確率で再試行する */
static void set_weird_bias_aura_fire(object_type *o_ptr)
{
    if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
        add_flag(o_ptr->art_flags, TR_SH_FIRE);
    else
        random_resistance(o_ptr);

    if (!o_ptr->artifact_bias)
        o_ptr->artifact_bias = BIAS_FIRE;
}

/* 一定確率で再試行する */
static void set_weird_bias_reflection(object_type *o_ptr)
{
    if (o_ptr->tval == TV_SHIELD || o_ptr->tval == TV_CLOAK || o_ptr->tval == TV_HELM || o_ptr->tval == TV_HARD_ARMOR) {
        add_flag(o_ptr->art_flags, TR_REFLECT);
        return;
    }

    random_resistance(o_ptr);
}

/* 一定確率で再試行する */
static void set_weird_bias_aura_cold(object_type *o_ptr)
{
    if (o_ptr->tval >= TV_CLOAK && o_ptr->tval <= TV_HARD_ARMOR)
        add_flag(o_ptr->art_flags, TR_SH_COLD);
    else
        random_resistance(o_ptr);

    if (!o_ptr->artifact_bias)
        o_ptr->artifact_bias = BIAS_COLD;
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
    if (switch_random_art_resistance(o_ptr))
        return;

    switch (randint1(42)) {
    case 1:
        set_weird_bias_acid(o_ptr);
        break;
    case 2:
        set_weird_bias_elec(o_ptr);
        break;
    case 3:
        set_weird_bias_cold(o_ptr);
        break;
    case 4:
        set_weird_bias_fire(o_ptr);
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
        set_bias_pois(o_ptr);
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
        set_weird_bias_aura_elec(o_ptr);
        break;
    case 40:
        set_weird_bias_aura_fire(o_ptr);
        break;
    case 41:
        set_weird_bias_reflection(o_ptr);
        break;
    case 42:
        set_weird_bias_aura_cold(o_ptr);
        break;
    }
}
