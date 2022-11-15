/*!
 * @file random-art-misc.cpp
 * @brief ランダムアーティファクト生成のその他特性バイアス付け実装 / Artifact code
 */

#include "artifact/random-art-misc.h"
#include "artifact/random-art-bias-types.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object/tval-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

static bool invest_misc_ranger(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SUST_CON)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SUST_CON);
    return one_in_(2);
}

static bool invest_misc_strength(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SUST_STR)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SUST_STR);
    return one_in_(2);
}

static bool invest_misc_wisdom(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SUST_WIS)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SUST_WIS);
    return one_in_(2);
}

static bool invest_misc_intelligence(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SUST_INT)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SUST_INT);
    return one_in_(2);
}

static bool invest_misc_dexterity(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SUST_DEX)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SUST_DEX);
    return one_in_(2);
}

static bool invest_misc_constitution(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SUST_CON)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SUST_CON);
    return one_in_(2);
}

static bool invest_misc_charisma(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_SUST_CHR)) {
        return false;
    }

    o_ptr->art_flags.set(TR_SUST_CHR);
    return one_in_(2);
}

static bool invest_misc_chaos(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_TELEPORT)) {
        return false;
    }

    o_ptr->art_flags.set(TR_TELEPORT);
    return one_in_(2);
}

static bool invest_misc_res_curse(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_RES_CURSE)) {
        return false;
    }

    o_ptr->art_flags.set(TR_RES_CURSE);
    return one_in_(2);
}

/*!
 * @brief アーティファクトのバイアス値に基づいて特性を付与する
 * @param o_ptr 対象のオブジェクト構造体への参照ポインタ
 * @return 処理続行ならFALSE、打ち切るならTRUE
 */
static bool switch_misc_bias(ItemEntity *o_ptr)
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
        if (o_ptr->art_flags.has_not(TR_LITE_1)) {
            o_ptr->art_flags.set(TR_LITE_1);
        }

        return false;
    default:
        return false;
    }
}

static void invest_misc_hit_dice(ItemEntity *o_ptr)
{
    o_ptr->art_flags.set(TR_SHOW_MODS);
    HIT_PROB bonus_h = 4 + (HIT_PROB)randint1(11);
    int bonus_d = 4 + (int)randint1(11);
    if ((o_ptr->tval != ItemKindType::SWORD) && (o_ptr->tval != ItemKindType::POLEARM) && (o_ptr->tval != ItemKindType::HAFTED) && (o_ptr->tval != ItemKindType::DIGGING) && (o_ptr->tval != ItemKindType::GLOVES) && (o_ptr->tval != ItemKindType::RING)) {
        bonus_h /= 2;
        bonus_d /= 2;
    }

    o_ptr->to_h += bonus_h;
    o_ptr->to_d += bonus_d;
}

static void invest_misc_string_esp(ItemEntity *o_ptr)
{
    switch (randint1(3)) {
    case 1:
        o_ptr->art_flags.set(TR_ESP_EVIL);
        if (!o_ptr->artifact_bias && one_in_(3)) {
            o_ptr->artifact_bias = BIAS_LAW;
        }

        return;
    case 2:
        o_ptr->art_flags.set(TR_ESP_NONLIVING);
        if (!o_ptr->artifact_bias && one_in_(3)) {
            o_ptr->artifact_bias = BIAS_MAGE;
        }

        return;
    case 3:
        o_ptr->art_flags.set(TR_TELEPATHY);
        if (!o_ptr->artifact_bias && one_in_(9)) {
            o_ptr->artifact_bias = BIAS_MAGE;
        }

        return;
    }
}

static void switch_investment_weak_esps(ItemEntity *o_ptr, const int *idx, const int n)
{
    switch (idx[n]) {
    case 1:
        o_ptr->art_flags.set(TR_ESP_ANIMAL);
        if (!o_ptr->artifact_bias && one_in_(4)) {
            o_ptr->artifact_bias = BIAS_RANGER;
        }

        break;
    case 2:
        o_ptr->art_flags.set(TR_ESP_UNDEAD);
        if (!o_ptr->artifact_bias && one_in_(3)) {
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        } else if (!o_ptr->artifact_bias && one_in_(6)) {
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
        }

        break;
    case 3:
        o_ptr->art_flags.set(TR_ESP_DEMON);
        break;
    case 4:
        o_ptr->art_flags.set(TR_ESP_ORC);
        break;
    case 5:
        o_ptr->art_flags.set(TR_ESP_TROLL);
        break;
    case 6:
        o_ptr->art_flags.set(TR_ESP_GIANT);
        break;
    case 7:
        o_ptr->art_flags.set(TR_ESP_DRAGON);
        break;
    case 8:
        o_ptr->art_flags.set(TR_ESP_HUMAN);
        if (!o_ptr->artifact_bias && one_in_(6)) {
            o_ptr->artifact_bias = BIAS_ROGUE;
        }

        break;
    case 9:
        o_ptr->art_flags.set(TR_ESP_GOOD);
        if (!o_ptr->artifact_bias && one_in_(3)) {
            o_ptr->artifact_bias = BIAS_LAW;
        }

        break;
    case 10:
        o_ptr->art_flags.set(TR_ESP_UNIQUE);
        if (!o_ptr->artifact_bias && one_in_(3)) {
            o_ptr->artifact_bias = BIAS_LAW;
        }

        break;
    }
}

static void invest_misc_weak_esps(ItemEntity *o_ptr)
{
    int idx[3];
    idx[0] = randint1(10);
    idx[1] = randint1(9);
    if (idx[1] >= idx[0]) {
        idx[1]++;
    }

    idx[2] = randint1(8);
    if (idx[2] >= idx[0]) {
        idx[2]++;
    }

    if (idx[2] >= idx[1]) {
        idx[2]++;
    }

    int n = randint1(3);
    while (n--) {
        switch_investment_weak_esps(o_ptr, idx, n);
    }
}

/*!
 * @brief ランダムアーティファクト生成中、対象のオブジェクトにその他特性を付加する。/ Add one misc flag on generation of randam artifact.
 * @details 優先的に付加される耐性がランダムアーティファクトバイアスに依存して存在する。
 * 原則的候補は各種能力維持、永久光源+1、麻痺知らず、経験値維持、浮遊、透明視、急回復、遅消化、
 * 乱テレポート、反魔法、反テレポート、警告、テレパシー、各種ESP、一部装備に殺戮修正。
 * @attention オブジェクトのtval、svalに依存したハードコーディング処理がある。
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 */
void random_misc(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    if (switch_misc_bias(o_ptr)) {
        return;
    }

    switch (randint1(34)) {
    case 1:
        o_ptr->art_flags.set(TR_SUST_STR);
        if (!o_ptr->artifact_bias) {
            o_ptr->artifact_bias = BIAS_STR;
        }

        break;
    case 2:
        o_ptr->art_flags.set(TR_SUST_INT);
        if (!o_ptr->artifact_bias) {
            o_ptr->artifact_bias = BIAS_INT;
        }

        break;
    case 3:
        o_ptr->art_flags.set(TR_SUST_WIS);
        if (!o_ptr->artifact_bias) {
            o_ptr->artifact_bias = BIAS_WIS;
        }

        break;
    case 4:
        o_ptr->art_flags.set(TR_SUST_DEX);
        if (!o_ptr->artifact_bias) {
            o_ptr->artifact_bias = BIAS_DEX;
        }

        break;
    case 5:
        o_ptr->art_flags.set(TR_SUST_CON);
        if (!o_ptr->artifact_bias) {
            o_ptr->artifact_bias = BIAS_CON;
        }

        break;
    case 6:
        o_ptr->art_flags.set(TR_SUST_CHR);
        if (!o_ptr->artifact_bias) {
            o_ptr->artifact_bias = BIAS_CHR;
        }

        break;
    case 7:
    case 8:
    case 14:
        o_ptr->art_flags.set(TR_FREE_ACT);
        break;
    case 9:
        o_ptr->art_flags.set(TR_HOLD_EXP);
        if (!o_ptr->artifact_bias && one_in_(5)) {
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        } else if (!o_ptr->artifact_bias && one_in_(6)) {
            o_ptr->artifact_bias = BIAS_NECROMANTIC;
        }

        break;
    case 10:
    case 11:
        o_ptr->art_flags.set(TR_LITE_1);
        break;
    case 12:
    case 13:
        o_ptr->art_flags.set(TR_LEVITATION);
        break;
    case 15:
    case 16:
    case 17:
        o_ptr->art_flags.set(TR_SEE_INVIS);
        break;
    case 19:
    case 20:
        o_ptr->art_flags.set(TR_SLOW_DIGEST);
        break;
    case 21:
    case 22:
        o_ptr->art_flags.set(TR_REGEN);
        break;
    case 23:
        o_ptr->art_flags.set(TR_TELEPORT);
        break;
    case 24:
    case 25:
    case 26:
        if (o_ptr->is_armour()) {
            random_misc(player_ptr, o_ptr);
        } else {
            o_ptr->to_a = 4 + randint1(11);
        }

        break;
    case 27:
    case 28:
    case 29:
        invest_misc_hit_dice(o_ptr);
        break;
    case 30:
        o_ptr->art_flags.set(TR_NO_MAGIC);
        break;
    case 31:
        o_ptr->art_flags.set(TR_NO_TELE);
        break;
    case 32:
        o_ptr->art_flags.set(TR_WARNING);
        break;
    case 18:
        invest_misc_string_esp(o_ptr);
        break;
    case 33:
        invest_misc_weak_esps(o_ptr);
        break;
    case 34:
        invest_misc_res_curse(o_ptr);
        break;
    }
}
