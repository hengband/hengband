﻿/*!
 * @brief ランダムアーティファクトの生成 / Artifact code
 * @date 2020/07/14
 * @author Hourier
 */

#include "artifact/random-art-generator.h"
#include "artifact/random-art-activation.h"
#include "artifact/random-art-bias-types.h"
#include "artifact/random-art-characteristics.h"
#include "artifact/random-art-misc.h"
#include "artifact/random-art-pval-investor.h"
#include "artifact/random-art-resistance.h"
#include "artifact/random-art-slay.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "flavor/object-flavor.h"
#include "game-option/cheat-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/object-kind-hook.h"
#include "object/object-kind.h"
#include "object/object-value-calc.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "player-info/avatar.h"
#include "sv-definition/sv-weapon-types.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#include "wizard/artifact-bias-table.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

static bool weakening_artifact(player_type *player_ptr, object_type *o_ptr)
{
    KIND_OBJECT_IDX k_idx = lookup_kind(o_ptr->tval, o_ptr->sval);
    object_kind *k_ptr = &k_info[k_idx];
    BIT_FLAGS flgs[TR_FLAG_SIZE];
    object_flags(player_ptr, o_ptr, flgs);

    if (has_flag(flgs, TR_KILL_EVIL)) {
        remove_flag(o_ptr->art_flags, TR_KILL_EVIL);
        add_flag(o_ptr->art_flags, TR_SLAY_EVIL);
        return TRUE;
    }

    if (k_ptr->dd < o_ptr->dd) {
        o_ptr->dd--;
        return TRUE;
    }

    if (k_ptr->ds < o_ptr->ds) {
        o_ptr->ds--;
        return TRUE;
    }

    if (o_ptr->to_d > 10) {
        o_ptr->to_d = o_ptr->to_d - damroll(1, 6);
        if (o_ptr->to_d < 10) {
            o_ptr->to_d = 10;
        }

        return TRUE;
    }

    return FALSE;
}

static void set_artifact_bias(player_type *player_ptr, object_type *o_ptr, int *warrior_artifact_bias)
{
    switch (player_ptr->pclass) {
    case CLASS_WARRIOR:
    case CLASS_BERSERKER:
    case CLASS_ARCHER:
    case CLASS_SAMURAI:
    case CLASS_CAVALRY:
    case CLASS_SMITH:
        o_ptr->artifact_bias = BIAS_WARRIOR;
        break;
    case CLASS_MAGE:
    case CLASS_HIGH_MAGE:
    case CLASS_SORCERER:
    case CLASS_MAGIC_EATER:
    case CLASS_BLUE_MAGE:
        o_ptr->artifact_bias = BIAS_MAGE;
        break;
    case CLASS_PRIEST:
        o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case CLASS_ROGUE:
    case CLASS_NINJA:
        o_ptr->artifact_bias = BIAS_ROGUE;
        *warrior_artifact_bias = 25;
        break;
    case CLASS_RANGER:
    case CLASS_SNIPER:
        o_ptr->artifact_bias = BIAS_RANGER;
        *warrior_artifact_bias = 30;
        break;
    case CLASS_PALADIN:
        o_ptr->artifact_bias = BIAS_PRIESTLY;
        *warrior_artifact_bias = 40;
        break;
    case CLASS_WARRIOR_MAGE:
    case CLASS_RED_MAGE:
        o_ptr->artifact_bias = BIAS_MAGE;
        *warrior_artifact_bias = 40;
        break;
    case CLASS_CHAOS_WARRIOR:
        o_ptr->artifact_bias = BIAS_CHAOS;
        *warrior_artifact_bias = 40;
        break;
    case CLASS_MONK:
    case CLASS_FORCETRAINER:
        o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case CLASS_MINDCRAFTER:
    case CLASS_BARD:
        if (randint1(5) > 2)
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case CLASS_TOURIST:
        if (randint1(5) > 2)
            o_ptr->artifact_bias = BIAS_WARRIOR;
        break;
    case CLASS_IMITATOR:
        if (randint1(2) > 1)
            o_ptr->artifact_bias = BIAS_RANGER;
        break;
    case CLASS_BEASTMASTER:
        o_ptr->artifact_bias = BIAS_CHR;
        *warrior_artifact_bias = 50;
        break;
    case CLASS_MIRROR_MASTER:
        if (randint1(4) > 1)
            o_ptr->artifact_bias = BIAS_MAGE;
        else
            o_ptr->artifact_bias = BIAS_ROGUE;
        
        break;
    }
}

static void decide_warrior_bias(player_type *player_ptr, object_type *o_ptr, const bool a_scroll)
{
    int warrior_artifact_bias = 0;
    if (a_scroll && one_in_(4))
        set_artifact_bias(player_ptr, o_ptr, &warrior_artifact_bias);

    if (a_scroll && (randint1(100) <= warrior_artifact_bias))
        o_ptr->artifact_bias = BIAS_WARRIOR;
}

static bool decide_random_art_cursed(const bool a_scroll, object_type *o_ptr)
{
    if (!a_scroll && one_in_(A_CURSED))
        return TRUE;

    if (((o_ptr->tval == TV_AMULET) || (o_ptr->tval == TV_RING)) && object_is_cursed(o_ptr))
        return TRUE;

    return FALSE;
}

static int decide_random_art_power(const bool a_cursed)
{
    int powers = randint1(5) + 1;
    while (one_in_(powers) || one_in_(7) || one_in_(10))
        powers++;

    if (!a_cursed && one_in_(WEIRD_LUCK))
        powers *= 2;

    if (a_cursed)
        powers /= 2;

    return powers;
}

static void invest_powers(player_type *player_ptr, object_type *o_ptr, int *powers, bool *has_pval, const bool a_cursed)
{
    int max_type = object_is_weapon_ammo(o_ptr) ? 7 : 5;
    while ((*powers)--) {
        switch (randint1(max_type)) {
        case 1:
        case 2:
            random_plus(o_ptr);
            *has_pval = TRUE;
            break;
        case 3:
        case 4:
            if (one_in_(2) && object_is_weapon_ammo(o_ptr) && (o_ptr->tval != TV_BOW)) {
                if (a_cursed && !one_in_(13))
                    break;
                if (one_in_(13)) {
                    if (one_in_(o_ptr->ds + 4))
                        o_ptr->ds++;
                } else {
                    if (one_in_(o_ptr->dd + 1))
                        o_ptr->dd++;
                }
            } else
                random_resistance(o_ptr);

            break;
        case 5:
            random_misc(player_ptr, o_ptr);
            break;
        case 6:
        case 7:
            random_slay(o_ptr);
            break;
        default:
            if (current_world_ptr->wizard)
                msg_print("Switch error in become_random_artifact!");

            (*powers)++;
        }
    };
}

static void strengthen_pval(object_type *o_ptr)
{
    if (has_flag(o_ptr->art_flags, TR_BLOWS)) {
        o_ptr->pval = randint1(2);
        if ((o_ptr->tval == TV_SWORD) && (o_ptr->sval == SV_HAYABUSA))
            o_ptr->pval++;
    } else {
        do {
            o_ptr->pval++;
        } while (o_ptr->pval < randint1(5) || one_in_(o_ptr->pval));
    }

    if ((o_ptr->pval > 4) && !one_in_(WEIRD_LUCK))
        o_ptr->pval = 4;
}

/*!
 * @brief 防具ならばAC修正、武具なら殺戮修正を付与する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトを示すアイテムへの参照ポインタ
 * @return なし
 */
static void invest_positive_modified_value(player_type *player_ptr, object_type *o_ptr)
{
    if (object_is_armour(player_ptr, o_ptr)) {
        o_ptr->to_a += randint1(o_ptr->to_a > 19 ? 1 : 20 - o_ptr->to_a);
        return;
    }
    
    if (!object_is_weapon_ammo(o_ptr))
        return;

    o_ptr->to_h += randint1(o_ptr->to_h > 19 ? 1 : 20 - o_ptr->to_h);
    o_ptr->to_d += randint1(o_ptr->to_d > 19 ? 1 : 20 - o_ptr->to_d);
    if ((has_flag(o_ptr->art_flags, TR_WIS)) && (o_ptr->pval > 0))
        add_flag(o_ptr->art_flags, TR_BLESSED);
}

/*!
 * @brief 防具のAC修正が高すぎた場合に弱化させる
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトを示すアイテムへの参照ポインタ
 * @return なし
 */
static void invest_negative_modified_value(player_type *player_ptr, object_type *o_ptr)
{
    if (!object_is_armour(player_ptr, o_ptr))
        return;

    while ((o_ptr->to_d + o_ptr->to_h) > 20) {
        if (one_in_(o_ptr->to_d) && one_in_(o_ptr->to_h))
            break;

        o_ptr->to_d -= (HIT_POINT)randint0(3);
        o_ptr->to_h -= (HIT_PROB)randint0(3);
    }

    while ((o_ptr->to_d + o_ptr->to_h) > 10) {
        if (one_in_(o_ptr->to_d) || one_in_(o_ptr->to_h))
            break;

        o_ptr->to_d -= (HIT_POINT)randint0(3);
        o_ptr->to_h -= (HIT_PROB)randint0(3);
    }
}

static void reset_flags_poison_needle(object_type *o_ptr)
{
    if ((o_ptr->tval != TV_SWORD) || (o_ptr->sval != SV_POISON_NEEDLE))
        return;

    o_ptr->to_h = 0;
    o_ptr->to_d = 0;
    remove_flag(o_ptr->art_flags, TR_BLOWS);
    remove_flag(o_ptr->art_flags, TR_FORCE_WEAPON);
    remove_flag(o_ptr->art_flags, TR_SLAY_ANIMAL);
    remove_flag(o_ptr->art_flags, TR_SLAY_EVIL);
    remove_flag(o_ptr->art_flags, TR_SLAY_UNDEAD);
    remove_flag(o_ptr->art_flags, TR_SLAY_DEMON);
    remove_flag(o_ptr->art_flags, TR_SLAY_ORC);
    remove_flag(o_ptr->art_flags, TR_SLAY_TROLL);
    remove_flag(o_ptr->art_flags, TR_SLAY_GIANT);
    remove_flag(o_ptr->art_flags, TR_SLAY_DRAGON);
    remove_flag(o_ptr->art_flags, TR_KILL_DRAGON);
    remove_flag(o_ptr->art_flags, TR_SLAY_HUMAN);
    remove_flag(o_ptr->art_flags, TR_VORPAL);
    remove_flag(o_ptr->art_flags, TR_BRAND_POIS);
    remove_flag(o_ptr->art_flags, TR_BRAND_ACID);
    remove_flag(o_ptr->art_flags, TR_BRAND_ELEC);
    remove_flag(o_ptr->art_flags, TR_BRAND_FIRE);
    remove_flag(o_ptr->art_flags, TR_BRAND_COLD);
}

static int decide_random_art_power_level(object_type *o_ptr, const bool a_cursed, const int total_flags)
{
    if (object_is_weapon_ammo(o_ptr)) {
        if (a_cursed)
            return 0;

        if (total_flags < 20000)
            return 1;

        if (total_flags < 45000)
            return 2;

        return 3;
    }

    if (a_cursed)
        return 0;

    if (total_flags < 15000)
        return 1;

    if (total_flags < 35000)
        return 2;

    return 3;
}

static void name_unnatural_random_artifact(player_type *player_ptr, object_type *o_ptr, const bool a_scroll, const int power_level, GAME_TEXT *new_name)
{
    if (!a_scroll) {
        get_random_name(o_ptr, new_name, object_is_armour(player_ptr, o_ptr), power_level);
        return;
    }

    GAME_TEXT dummy_name[MAX_NLEN] = "";
    concptr ask_msg = _("このアーティファクトを何と名付けますか？", "What do you want to call the artifact? ");
    object_aware(player_ptr, o_ptr);
    object_known(o_ptr);
    o_ptr->ident |= IDENT_FULL_KNOWN;
    o_ptr->art_name = quark_add("");
    (void)screen_object(player_ptr, o_ptr, 0L);
    if (!get_string(ask_msg, dummy_name, sizeof dummy_name) || !dummy_name[0]) {
        if (one_in_(2)) {
            get_table_sindarin_aux(dummy_name);
        } else {
            get_table_name_aux(dummy_name);
        }
    }

    sprintf(new_name, _("《%s》", "'%s'"), dummy_name);
    chg_virtue(player_ptr, V_INDIVIDUALISM, 2);
    chg_virtue(player_ptr, V_ENCHANT, 5);
}

static void generate_unnatural_random_artifact(player_type *player_ptr, object_type *o_ptr, const bool a_scroll, const int power_level, const int max_powers, const int total_flags)
{
    GAME_TEXT new_name[1024];
    strcpy(new_name, "");
    name_unnatural_random_artifact(player_ptr, o_ptr, a_scroll, power_level, new_name);
    o_ptr->art_name = quark_add(new_name);
    msg_format_wizard(player_ptr, CHEAT_OBJECT,
        _("パワー %d で 価値%ld のランダムアーティファクト生成 バイアスは「%s」", "Random artifact generated - Power:%d Value:%d Bias:%s."), max_powers,
        total_flags, artifact_bias_name[o_ptr->artifact_bias]);
    player_ptr->window_flags |= PW_INVEN | PW_EQUIP;
}

/*!
 * @brief ランダムアーティファクト生成のメインルーチン
 * @details 既に生成が済んでいるオブジェクトの構造体を、アーティファクトとして強化する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @param a_scroll アーティファクト生成の巻物上の処理。呪いのアーティファクトが生成対象外となる。
 * @return 常にTRUE(1)を返す
 */
bool become_random_artifact(player_type *player_ptr, object_type *o_ptr, bool a_scroll)
{
    o_ptr->artifact_bias = 0;
    o_ptr->name1 = 0;
    o_ptr->name2 = 0;
    for (int i = 0; i < TR_FLAG_SIZE; i++)
        o_ptr->art_flags[i] |= k_info[o_ptr->k_idx].flags[i];

    bool has_pval = o_ptr->pval != 0;
    decide_warrior_bias(player_ptr, o_ptr, a_scroll);

    bool a_cursed = decide_random_art_cursed(a_scroll, o_ptr);
    int powers = decide_random_art_power(a_cursed);
    int max_powers = powers;
    invest_powers(player_ptr, o_ptr, &powers, &has_pval, a_cursed);
    if (has_pval)
        strengthen_pval(o_ptr);

    invest_positive_modified_value(player_ptr, o_ptr);
    add_flag(o_ptr->art_flags, TR_IGNORE_ACID);
    add_flag(o_ptr->art_flags, TR_IGNORE_ELEC);
    add_flag(o_ptr->art_flags, TR_IGNORE_FIRE);
    add_flag(o_ptr->art_flags, TR_IGNORE_COLD);

    s32b total_flags = flag_cost(player_ptr, o_ptr, o_ptr->pval);
    if (a_cursed)
        curse_artifact(player_ptr, o_ptr);

    if (!a_cursed && one_in_(object_is_armour(player_ptr, o_ptr) ? ACTIVATION_CHANCE * 2 : ACTIVATION_CHANCE)) {
        o_ptr->xtra2 = 0;
        give_activation_power(o_ptr);
    }

    invest_negative_modified_value(player_ptr, o_ptr);
    if (((o_ptr->artifact_bias == BIAS_MAGE) || (o_ptr->artifact_bias == BIAS_INT)) && (o_ptr->tval == TV_GLOVES))
        add_flag(o_ptr->art_flags, TR_FREE_ACT);

    reset_flags_poison_needle(o_ptr);
    int power_level = decide_random_art_power_level(o_ptr, a_cursed, total_flags);
    while (has_extreme_damage_rate(player_ptr, o_ptr) && !one_in_(SWORDFISH_LUCK))
        weakening_artifact(player_ptr, o_ptr);

    generate_unnatural_random_artifact(player_ptr, o_ptr, a_scroll, power_level, max_powers, total_flags);
    return TRUE;
}
