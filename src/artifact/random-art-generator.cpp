/*!
 * @file random-art-generator.cpp
 * @brief ランダムアーティファクトの生成メイン定義 / Artifact code
 * @date 2020/07/14
 * @author Hourier
 */

#include "artifact/random-art-generator.h"
#include "artifact/fixed-art-types.h"
#include "artifact/random-art-activation.h"
#include "artifact/random-art-bias-types.h"
#include "artifact/random-art-characteristics.h"
#include "artifact/random-art-effects.h"
#include "artifact/random-art-misc.h"
#include "artifact/random-art-pval-investor.h"
#include "artifact/random-art-resistance.h"
#include "artifact/random-art-slay.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/window-redrawer.h"
#include "flavor/object-flavor.h"
#include "game-option/cheat-types.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-armor.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "object/object-kind-hook.h"
#include "object/object-value-calc.h"
#include "object/tval-types.h"
#include "perception/identification.h"
#include "perception/object-perception.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "view/display-messages.h"
#include "wizard/artifact-bias-table.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

static bool weakening_artifact(ItemEntity *o_ptr)
{
    const auto bi_id = lookup_baseitem_id(o_ptr->bi_key);
    const auto &baseitem = baseitems_info[bi_id];
    auto flags = object_flags(o_ptr);

    if (flags.has(TR_KILL_EVIL)) {
        o_ptr->art_flags.reset(TR_KILL_EVIL);
        o_ptr->art_flags.set(TR_SLAY_EVIL);
        return true;
    }

    if (baseitem.dd < o_ptr->dd) {
        o_ptr->dd--;
        return true;
    }

    if (baseitem.ds < o_ptr->ds) {
        o_ptr->ds--;
        return true;
    }

    if (o_ptr->to_d > 10) {
        o_ptr->to_d = o_ptr->to_d - damroll(1, 6);
        if (o_ptr->to_d < 10) {
            o_ptr->to_d = 10;
        }

        return true;
    }

    return false;
}

static void set_artifact_bias(PlayerType *player_ptr, ItemEntity *o_ptr, int *warrior_artifact_bias)
{
    switch (player_ptr->pclass) {
    case PlayerClassType::WARRIOR:
    case PlayerClassType::BERSERKER:
    case PlayerClassType::ARCHER:
    case PlayerClassType::SAMURAI:
    case PlayerClassType::CAVALRY:
    case PlayerClassType::SMITH:
        o_ptr->artifact_bias = BIAS_WARRIOR;
        break;
    case PlayerClassType::MAGE:
    case PlayerClassType::HIGH_MAGE:
    case PlayerClassType::SORCERER:
    case PlayerClassType::MAGIC_EATER:
    case PlayerClassType::BLUE_MAGE:
        o_ptr->artifact_bias = BIAS_MAGE;
        break;
    case PlayerClassType::PRIEST:
        o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case PlayerClassType::ROGUE:
    case PlayerClassType::NINJA:
        o_ptr->artifact_bias = BIAS_ROGUE;
        *warrior_artifact_bias = 25;
        break;
    case PlayerClassType::RANGER:
    case PlayerClassType::SNIPER:
        o_ptr->artifact_bias = BIAS_RANGER;
        *warrior_artifact_bias = 30;
        break;
    case PlayerClassType::PALADIN:
        o_ptr->artifact_bias = BIAS_PRIESTLY;
        *warrior_artifact_bias = 40;
        break;
    case PlayerClassType::WARRIOR_MAGE:
    case PlayerClassType::RED_MAGE:
        o_ptr->artifact_bias = BIAS_MAGE;
        *warrior_artifact_bias = 40;
        break;
    case PlayerClassType::CHAOS_WARRIOR:
        o_ptr->artifact_bias = BIAS_CHAOS;
        *warrior_artifact_bias = 40;
        break;
    case PlayerClassType::MONK:
    case PlayerClassType::FORCETRAINER:
        o_ptr->artifact_bias = BIAS_PRIESTLY;
        break;
    case PlayerClassType::MINDCRAFTER:
    case PlayerClassType::BARD:
        if (randint1(5) > 2) {
            o_ptr->artifact_bias = BIAS_PRIESTLY;
        }
        break;
    case PlayerClassType::TOURIST:
        if (randint1(5) > 2) {
            o_ptr->artifact_bias = BIAS_WARRIOR;
        }
        break;
    case PlayerClassType::IMITATOR:
        if (randint1(2) > 1) {
            o_ptr->artifact_bias = BIAS_RANGER;
        }
        break;
    case PlayerClassType::BEASTMASTER:
        o_ptr->artifact_bias = BIAS_CHR;
        *warrior_artifact_bias = 50;
        break;
    case PlayerClassType::MIRROR_MASTER:
        if (randint1(4) > 1) {
            o_ptr->artifact_bias = BIAS_MAGE;
        } else {
            o_ptr->artifact_bias = BIAS_ROGUE;
        }
        break;
    case PlayerClassType::ELEMENTALIST:
        o_ptr->artifact_bias = one_in_(2) ? BIAS_MAGE : BIAS_INT;
        break;

    case PlayerClassType::MAX:
        break;
    }
}

static void decide_warrior_bias(PlayerType *player_ptr, ItemEntity *o_ptr, const bool a_scroll)
{
    int warrior_artifact_bias = 0;
    if (a_scroll && one_in_(4)) {
        set_artifact_bias(player_ptr, o_ptr, &warrior_artifact_bias);
    }

    if (a_scroll && (randint1(100) <= warrior_artifact_bias)) {
        o_ptr->artifact_bias = BIAS_WARRIOR;
    }
}

static bool decide_random_art_cursed(const bool a_scroll, ItemEntity *o_ptr)
{
    constexpr auto chance_cursed = 13;
    if (!a_scroll && one_in_(chance_cursed)) {
        return true;
    }

    const auto tval = o_ptr->bi_key.tval();
    if (((tval == ItemKindType::AMULET) || (tval == ItemKindType::RING)) && o_ptr->is_cursed()) {
        return true;
    }

    return false;
}

static int decide_random_art_power(const bool a_cursed)
{
    int powers = randint1(5) + 1;
    while (one_in_(powers) || one_in_(7) || one_in_(10)) {
        powers++;
    }

    if (!a_cursed && one_in_(CHANCE_STRENGTHENING)) {
        powers *= 2;
    }

    if (a_cursed) {
        powers /= 2;
    }

    return powers;
}

static void invest_powers(PlayerType *player_ptr, ItemEntity *o_ptr, int *powers, bool *has_pval, const bool a_cursed)
{
    int max_type = o_ptr->is_weapon_ammo() ? 7 : 5;
    while ((*powers)--) {
        switch (randint1(max_type)) {
        case 1:
        case 2:
            random_plus(o_ptr);
            *has_pval = true;
            break;
        case 3:
        case 4:
            if (one_in_(2) && o_ptr->is_weapon_ammo() && (o_ptr->bi_key.tval() != ItemKindType::BOW)) {
                if (a_cursed && !one_in_(13)) {
                    break;
                }
                if (one_in_(13)) {
                    if (one_in_(o_ptr->ds + 4)) {
                        o_ptr->ds++;
                    }
                } else {
                    if (one_in_(o_ptr->dd + 1)) {
                        o_ptr->dd++;
                    }
                }
            } else {
                random_resistance(o_ptr);
            }

            break;
        case 5:
            random_misc(player_ptr, o_ptr);
            break;
        case 6:
        case 7:
            random_slay(o_ptr);
            break;
        default:
            if (w_ptr->wizard) {
                msg_print("Switch error in become_random_artifact!");
            }

            (*powers)++;
        }
    };
}

static void strengthen_pval(ItemEntity *o_ptr)
{
    if (o_ptr->art_flags.has(TR_BLOWS)) {
        o_ptr->pval = randint1(2);
        if (o_ptr->bi_key == BaseitemKey(ItemKindType::SWORD, SV_HAYABUSA)) {
            o_ptr->pval++;
        }
    } else {
        do {
            o_ptr->pval++;
        } while (o_ptr->pval < randint1(5) || one_in_(o_ptr->pval));
    }

    if ((o_ptr->pval > 4) && !one_in_(CHANCE_STRENGTHENING)) {
        o_ptr->pval = 4;
    }
}

/*!
 * @brief 防具ならばAC修正、武具なら殺戮修正を付与する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトを示すアイテムへの参照ポインタ
 */
static void invest_positive_modified_value(ItemEntity *o_ptr)
{
    if (o_ptr->is_protector()) {
        o_ptr->to_a += randint1(o_ptr->to_a > 19 ? 1 : 20 - o_ptr->to_a);
        return;
    }

    if (!o_ptr->is_weapon_ammo()) {
        return;
    }

    o_ptr->to_h += randint1(o_ptr->to_h > 19 ? 1 : 20 - o_ptr->to_h);
    o_ptr->to_d += randint1(o_ptr->to_d > 19 ? 1 : 20 - o_ptr->to_d);
    if ((o_ptr->art_flags.has(TR_WIS)) && (o_ptr->pval > 0)) {
        o_ptr->art_flags.set(TR_BLESSED);
    }
}

/*!
 * @brief 防具のAC修正が高すぎた場合に弱化させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr ランダムアーティファクトを示すアイテムへの参照ポインタ
 */
static void invest_negative_modified_value(ItemEntity *o_ptr)
{
    if (!o_ptr->is_protector()) {
        return;
    }

    while ((o_ptr->to_d + o_ptr->to_h) > 20) {
        if (one_in_(o_ptr->to_d) && one_in_(o_ptr->to_h)) {
            break;
        }

        o_ptr->to_d -= (int)randint0(3);
        o_ptr->to_h -= (HIT_PROB)randint0(3);
    }

    while ((o_ptr->to_d + o_ptr->to_h) > 10) {
        if (one_in_(o_ptr->to_d) || one_in_(o_ptr->to_h)) {
            break;
        }

        o_ptr->to_d -= (int)randint0(3);
        o_ptr->to_h -= (HIT_PROB)randint0(3);
    }
}

static void reset_flags_poison_needle(ItemEntity *o_ptr)
{
    if (o_ptr->bi_key != BaseitemKey(ItemKindType::SWORD, SV_POISON_NEEDLE)) {
        return;
    }

    o_ptr->to_h = 0;
    o_ptr->to_d = 0;
    o_ptr->art_flags.reset(TR_BLOWS);
    o_ptr->art_flags.reset(TR_FORCE_WEAPON);
    o_ptr->art_flags.reset(TR_SLAY_ANIMAL);
    o_ptr->art_flags.reset(TR_SLAY_EVIL);
    o_ptr->art_flags.reset(TR_SLAY_UNDEAD);
    o_ptr->art_flags.reset(TR_SLAY_DEMON);
    o_ptr->art_flags.reset(TR_SLAY_ORC);
    o_ptr->art_flags.reset(TR_SLAY_TROLL);
    o_ptr->art_flags.reset(TR_SLAY_GIANT);
    o_ptr->art_flags.reset(TR_SLAY_DRAGON);
    o_ptr->art_flags.reset(TR_KILL_DRAGON);
    o_ptr->art_flags.reset(TR_SLAY_HUMAN);
    o_ptr->art_flags.reset(TR_VORPAL);
    o_ptr->art_flags.reset(TR_BRAND_POIS);
    o_ptr->art_flags.reset(TR_BRAND_ACID);
    o_ptr->art_flags.reset(TR_BRAND_ELEC);
    o_ptr->art_flags.reset(TR_BRAND_FIRE);
    o_ptr->art_flags.reset(TR_BRAND_COLD);
}

static int decide_random_art_power_level(ItemEntity *o_ptr, const bool a_cursed, const int total_flags)
{
    if (o_ptr->is_weapon_ammo()) {
        if (a_cursed) {
            return 0;
        }

        if (total_flags < 20000) {
            return 1;
        }

        if (total_flags < 45000) {
            return 2;
        }

        return 3;
    }

    if (a_cursed) {
        return 0;
    }

    if (total_flags < 15000) {
        return 1;
    }

    if (total_flags < 35000) {
        return 2;
    }

    return 3;
}

static std::string name_unnatural_random_artifact(PlayerType *player_ptr, ItemEntity *o_ptr, const bool a_scroll, const int power_level)
{
    if (!a_scroll) {
        return get_random_name(*o_ptr, o_ptr->is_protector(), power_level);
    }

    concptr ask_msg = _("このアーティファクトを何と名付けますか？", "What do you want to call the artifact? ");
    object_aware(player_ptr, o_ptr);
    object_known(o_ptr);
    o_ptr->ident |= IDENT_FULL_KNOWN;
    o_ptr->randart_name.reset();
    (void)screen_object(player_ptr, o_ptr, 0L);
    char new_name[160] = "";
    if (!get_string(ask_msg, new_name, sizeof new_name) || !new_name[0]) {
        if (one_in_(2)) {
            return get_table_sindarin_aux();
        } else {
            return get_table_name_aux();
        }
    }

    return std::string(_("《", "'")).append(new_name).append(_("》", "'"));
}

static void generate_unnatural_random_artifact(
    PlayerType *player_ptr, ItemEntity *o_ptr, const bool a_scroll, const int power_level, const int max_powers, const int total_flags)
{
    o_ptr->randart_name = name_unnatural_random_artifact(player_ptr, o_ptr, a_scroll, power_level);
    msg_format_wizard(player_ptr, CHEAT_OBJECT,
        _("パワー %d で 価値 %d のランダムアーティファクト生成 バイアスは「%s」", "Random artifact generated - Power:%d Value:%d Bias:%s."), max_powers,
        total_flags, artifact_bias_name[o_ptr->artifact_bias]);
    set_bits(player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_FLOOR_ITEM_LIST | PW_FOUND_ITEM_LIST);
}

/*!
 * @brief ランダムアーティファクト生成のメインルーチン
 * @details 既に生成が済んでいるオブジェクトの構造体を、アーティファクトとして強化する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 対象のオブジェクト構造体ポインタ
 * @param a_scroll アーティファクト生成の巻物上の処理。呪いのアーティファクトが生成対象外となる。
 * @return 常にTRUE(1)を返す
 */
bool become_random_artifact(PlayerType *player_ptr, ItemEntity *o_ptr, bool a_scroll)
{
    o_ptr->artifact_bias = 0;
    o_ptr->fixed_artifact_idx = FixedArtifactId::NONE;
    o_ptr->ego_idx = EgoType::NONE;
    o_ptr->art_flags |= baseitems_info[o_ptr->bi_id].flags;

    bool has_pval = o_ptr->pval != 0;
    decide_warrior_bias(player_ptr, o_ptr, a_scroll);

    bool a_cursed = decide_random_art_cursed(a_scroll, o_ptr);
    int powers = decide_random_art_power(a_cursed);
    int max_powers = powers;
    invest_powers(player_ptr, o_ptr, &powers, &has_pval, a_cursed);
    if (has_pval) {
        strengthen_pval(o_ptr);
    }

    invest_positive_modified_value(o_ptr);
    o_ptr->art_flags.set(TR_IGNORE_ACID);
    o_ptr->art_flags.set(TR_IGNORE_ELEC);
    o_ptr->art_flags.set(TR_IGNORE_FIRE);
    o_ptr->art_flags.set(TR_IGNORE_COLD);

    int32_t total_flags = flag_cost(o_ptr, o_ptr->pval);
    if (a_cursed) {
        curse_artifact(player_ptr, o_ptr);
    }

    constexpr auto activation_chance = 3;
    if (!a_cursed && one_in_(o_ptr->is_protector() ? activation_chance * 2 : activation_chance)) {
        o_ptr->activation_id = RandomArtActType::NONE;
        give_activation_power(o_ptr);
    }

    invest_negative_modified_value(o_ptr);
    if (((o_ptr->artifact_bias == BIAS_MAGE) || (o_ptr->artifact_bias == BIAS_INT)) && (o_ptr->bi_key.tval() == ItemKindType::GLOVES)) {
        o_ptr->art_flags.set(TR_FREE_ACT);
    }

    reset_flags_poison_needle(o_ptr);
    int power_level = decide_random_art_power_level(o_ptr, a_cursed, total_flags);
    constexpr auto chance_avoid_weakening = 6;
    while (has_extreme_damage_rate(player_ptr, o_ptr) && !one_in_(chance_avoid_weakening)) {
        weakening_artifact(o_ptr);
    }

    generate_unnatural_random_artifact(player_ptr, o_ptr, a_scroll, power_level, max_powers, total_flags);

    if (a_scroll) {
        chg_virtue(player_ptr, V_INDIVIDUALISM, 2);
        chg_virtue(player_ptr, V_ENCHANT, 5);
    }

    return true;
}
