﻿#include "artifact/random-art-misc.h"
#include "artifact/random-art-bias-types.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"

static bool invest_misc_ranger(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SUST_CON))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SUST_CON);
    return one_in_(2);
}

static bool invest_misc_strength(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SUST_STR))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SUST_STR);
    return one_in_(2);
}

static bool invest_misc_wisdom(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SUST_WIS))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SUST_WIS);
    return one_in_(2);
}

static bool invest_misc_intelligence(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SUST_INT))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SUST_INT);
    return one_in_(2);
}

static bool invest_misc_dexterity(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SUST_DEX))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SUST_DEX);
    return one_in_(2);
}

static bool invest_misc_constitution(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SUST_CON))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SUST_CON);
    return one_in_(2);
}

static bool invest_misc_charisma(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_SUST_CHR))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_SUST_CHR);
    return one_in_(2);
}

static bool invest_misc_chaos(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_TELEPORT))
        return FALSE;

    add_flag(o_ptr->art_flags, TR_TELEPORT);
    return one_in_(2);
}

/*!
 * @brief アーティファクトのバイアス値に基づいて特性を付与する
 * @param o_ptr 対象のオブジェクト構造体への参照ポインタ
 * @return 処理続行ならFALSE、打ち切るならTRUE
 */
static bool switch_misc_bias(object_type *o_ptr)
{
    switch (o_ptr->artifact_bias) {
    case BIAS_RANGER:
        return invest_misc_ranger(o_ptr);
    case BIAS_STR:
        return invest_misc_strength(o_ptr);
    case BIAS_INT:
        return invest_misc_intelligence(o_ptr);
    case BIAS_WIS:
        return invest_misc_wisdom(o_ptr);
    case BIAS_DEX:
        return invest_misc_dexterity(o_ptr);
    case BIAS_CON:
        return invest_misc_constitution(o_ptr);
    case BIAS_CHR:
        return invest_misc_charisma(o_ptr);
    case BIAS_CHAOS:
        return invest_misc_chaos(o_ptr);
    case BIAS_FIRE:
        if (!(has_flag(o_ptr->art_flags, TR_LITE_1)))
            add_flag(o_ptr->art_flags, TR_LITE_1);

        return FALSE;
    default:
        return FALSE;
    }
}

static void invest_misc_hit_dice(object_type *o_ptr)
{
    add_flag(o_ptr->art_flags, TR_SHOW_MODS);
    HIT_PROB bonus_h = 4 + (HIT_PROB)randint1(11);
    HIT_POINT bonus_d = 4 + (HIT_POINT)randint1(11);
    if ((o_ptr->tval != TV_SWORD) && (o_ptr->tval != TV_POLEARM) && (o_ptr->tval != TV_HAFTED) && (o_ptr->tval != TV_DIGGING) && (o_ptr->tval != TV_GLOVES)
        && (o_ptr->tval != TV_RING)) {
        bonus_h /= 2;
        bonus_d /= 2;
    }

    o_ptr->to_h += bonus_h;
    o_ptr->to_d += bonus_d;
}

static void invest_misc_string_esp(object_type *o_ptr)
{
    switch (randint1(3)) {
    case 1:
        add_flag(o_ptr->art_flags, TR_ESP_EVIL);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_LAW;

        return;
    case 2:
        add_flag(o_ptr->art_flags, TR_ESP_NONLIVING);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_MAGE;

        return;
    case 3:
        add_flag(o_ptr->art_flags, TR_TELEPATHY);
        if (!o_ptr->artifact_bias && one_in_(9))
            o_ptr->artifact_bias = BIAS_MAGE;

        return;
    }
}

static void switch_investment_weak_esps(object_type *o_ptr, const int *idx, const int n)
{
    switch (idx[n]) {
    case 1:
        add_flag(o_ptr->art_flags, TR_ESP_ANIMAL);
        if (!o_ptr->artifact_bias && one_in_(4))
            o_ptr->artifact_bias = BIAS_RANGER;

        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_ESP_UNDEAD);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        else if (!o_ptr->artifact_bias && one_in_(6))
            o_ptr->artifact_bias = BIAS_NECROMANTIC;

        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_ESP_DEMON);
        break;
    case 4:
        add_flag(o_ptr->art_flags, TR_ESP_ORC);
        break;
    case 5:
        add_flag(o_ptr->art_flags, TR_ESP_TROLL);
        break;
    case 6:
        add_flag(o_ptr->art_flags, TR_ESP_GIANT);
        break;
    case 7:
        add_flag(o_ptr->art_flags, TR_ESP_DRAGON);
        break;
    case 8:
        add_flag(o_ptr->art_flags, TR_ESP_HUMAN);
        if (!o_ptr->artifact_bias && one_in_(6))
            o_ptr->artifact_bias = BIAS_ROGUE;

        break;
    case 9:
        add_flag(o_ptr->art_flags, TR_ESP_GOOD);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_LAW;

        break;
    case 10:
        add_flag(o_ptr->art_flags, TR_ESP_UNIQUE);
        if (!o_ptr->artifact_bias && one_in_(3))
            o_ptr->artifact_bias = BIAS_LAW;

        break;
    }
}

static void invest_misc_weak_esps(object_type *o_ptr)
{
    int idx[3];
    idx[0] = randint1(10);
    idx[1] = randint1(9);
    if (idx[1] >= idx[0])
        idx[1]++;

    idx[2] = randint1(8);
    if (idx[2] >= idx[0])
        idx[2]++;

    if (idx[2] >= idx[1])
        idx[2]++;

    int n = randint1(3);
    while (n--)
        switch_investment_weak_esps(o_ptr, idx, n);
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにその他特性を付加する。/ Add one misc flag on generation of randam artifact.
 * @details 優先的に付加される耐性がランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は各種能力維持、永久光源+1、麻痺知らず、経験値維持、浮遊、透明視、急回復、遅消化、
 * 乱テレポート、反魔法、反テレポート、警告、テレパシー、各種ESP、一部装備に殺戮修正。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @return なし
 */
void random_misc(player_type *player_ptr, object_type *o_ptr)
{
    if (switch_misc_bias(o_ptr))
        return;

    switch (randint1(33)) {
    case 1:
        add_flag(o_ptr->art_flags, TR_SUST_STR);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_STR;

        break;
    case 2:
        add_flag(o_ptr->art_flags, TR_SUST_INT);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_INT;

        break;
    case 3:
        add_flag(o_ptr->art_flags, TR_SUST_WIS);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_WIS;

        break;
    case 4:
        add_flag(o_ptr->art_flags, TR_SUST_DEX);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_DEX;

        break;
    case 5:
        add_flag(o_ptr->art_flags, TR_SUST_CON);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_CON;

        break;
    case 6:
        add_flag(o_ptr->art_flags, TR_SUST_CHR);
        if (!o_ptr->artifact_bias)
            o_ptr->artifact_bias = BIAS_CHR;

        break;
    case 7:
    case 8:
    case 14:
        add_flag(o_ptr->art_flags, TR_FREE_ACT);
        break;
    case 9:
        add_flag(o_ptr->art_flags, TR_HOLD_EXP);
        if (!o_ptr->artifact_bias && one_in_(5))
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        else if (!o_ptr->artifact_bias && one_in_(6))
            o_ptr->artifact_bias = BIAS_NECROMANTIC;

        break;
    case 10:
    case 11:
        add_flag(o_ptr->art_flags, TR_LITE_1);
        break;
    case 12:
    case 13:
        add_flag(o_ptr->art_flags, TR_LEVITATION);
        break;
    case 15:
    case 16:
    case 17:
        add_flag(o_ptr->art_flags, TR_SEE_INVIS);
        break;
    case 19:
    case 20:
        add_flag(o_ptr->art_flags, TR_SLOW_DIGEST);
        break;
    case 21:
    case 22:
        add_flag(o_ptr->art_flags, TR_REGEN);
        break;
    case 23:
        add_flag(o_ptr->art_flags, TR_TELEPORT);
        break;
    case 24:
    case 25:
    case 26:
        if (object_is_armour(player_ptr, o_ptr))
            random_misc(player_ptr, o_ptr);
        else
            o_ptr->to_a = 4 + randint1(11);

        break;
    case 27:
    case 28:
    case 29:
        invest_misc_hit_dice(o_ptr);
        break;
    case 30:
        add_flag(o_ptr->art_flags, TR_NO_MAGIC);
        break;
    case 31:
        add_flag(o_ptr->art_flags, TR_NO_TELE);
        break;
    case 32:
        add_flag(o_ptr->art_flags, TR_WARNING);
        break;
    case 18:
        invest_misc_string_esp(o_ptr);
        break;
    case 33:
        invest_misc_weak_esps(o_ptr);
        break;
    }
}
