﻿/*!
 * @brief モンスターからプレーヤーへの直接攻撃をその種別において振り分ける
 * @date 2020/05/31
 * @author Hourier
 * @details 長い処理はインクルード先の別ファイルにて行っている
 */

#include "monster-attack/monster-attack-switcher.h"
#include "inventory/inventory-slot-types.h"
#include "mind/drs-types.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-lose.h"
#include "monster-attack/monster-attack-status.h"
#include "monster-attack/monster-eating.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-equipment.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief 毒ダメージを計算する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 * @detail 減衰の計算式がpoisではなくnukeなのは仕様 (1/3では減衰が強すぎると判断したため)
 */
static void calc_blow_poison(player_type *target_ptr, monap_type *monap_ptr)
{
    if (monap_ptr->explode)
        return;

    if (!(has_resist_pois(target_ptr) || is_oppose_pois(target_ptr)) && !check_multishadow(target_ptr)
        && set_poisoned(target_ptr, target_ptr->poisoned + randint1(monap_ptr->rlev) + 5))
        monap_ptr->obvious = TRUE;

    monap_ptr->damage = monap_ptr->damage * calc_nuke_damage_rate(target_ptr) / 100;
    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_POIS);
}

/*!
 * @brief 劣化ダメージを計算する (耐性があれば、(1d4 + 4) / 9になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_disenchant(player_type *target_ptr, monap_type *monap_ptr)
{
    if (monap_ptr->explode)
        return;

    if (!has_resist_disen(target_ptr) && !check_multishadow(target_ptr) && apply_disenchant(target_ptr, 0)) {
        update_creature(target_ptr);
        monap_ptr->obvious = TRUE;
    }

    if (has_resist_disen(target_ptr))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_DISEN);
}

/*!
 * @brief 魔道具吸収ダメージを計算する (消費魔力減少、呪文失敗率減少、魔道具使用能力向上があればそれぞれ-7.5%)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 * @detals 魔道具使用能力向上フラグがあれば、吸収対象のアイテムをスキャンされる回数が半分で済む
 */
static void calc_blow_un_power(player_type *target_ptr, monap_type *monap_ptr)
{
    int damage_ratio = 1000;
    if (has_dec_mana(target_ptr))
        damage_ratio -= 75;

    if (has_easy_spell(target_ptr))
        damage_ratio -= 75;

    bool is_magic_mastery = has_magic_mastery(target_ptr) != 0;
    if (is_magic_mastery)
        damage_ratio -= 75;

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 1000;
    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    int max_draining_item = is_magic_mastery ? 5 : 10;
    for (int i = 0; i < max_draining_item; i++) {
        INVENTORY_IDX i_idx = (INVENTORY_IDX)randint0(INVEN_PACK);
        monap_ptr->o_ptr = &target_ptr->inventory_list[i_idx];
        if (monap_ptr->o_ptr->k_idx == 0)
            continue;

        if (process_un_power(target_ptr, monap_ptr))
            break;
    }
}

/*!
 * @brief 盲目ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_blind(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_resist_blind(target_ptr))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    if (target_ptr->is_dead)
        return;

    process_blind_attack(target_ptr, monap_ptr);
    update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_BLIND);
}

/*!
 * @brief 混乱ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_confusion(player_type *target_ptr, monap_type *monap_ptr)
{
    if (monap_ptr->explode)
        return;

    if (has_resist_conf(target_ptr))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    if (target_ptr->is_dead)
        return;

    if (!has_resist_conf(target_ptr) && !check_multishadow(target_ptr) && set_confused(target_ptr, target_ptr->confused + 3 + randint1(monap_ptr->rlev)))
        monap_ptr->obvious = TRUE;

    update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_CONF);
}

/*!
 * @brief 恐怖ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_fear(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_resist_fear(target_ptr))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    if (target_ptr->is_dead)
        return;

    process_terrify_attack(target_ptr, monap_ptr);
    update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_FEAR);
}

/*!
 * @brief 麻痺ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_paralysis(player_type *target_ptr, monap_type *monap_ptr)
{
    if (has_free_act(target_ptr))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    if (target_ptr->is_dead)
        return;

    process_paralyze_attack(target_ptr, monap_ptr);
    update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_FREE);
}

/*!
 * @brief 経験値吸収ダメージを計算する (経験値保持と地獄耐性があれば、それぞれ-7.5%)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_drain_exp(player_type *target_ptr, monap_type *monap_ptr, const int drain_value, const int hold_exp_prob)
{
    s32b d = damroll(drain_value, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
    monap_ptr->obvious = TRUE;
    int damage_ratio = 1000;
    if (has_hold_exp(target_ptr))
        damage_ratio -= 75;

    if (has_resist_neth(target_ptr))
        damage_ratio -= 75;

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 1000;
    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    (void)drain_exp(target_ptr, d, d / 10, hold_exp_prob);
}

/*!
 * @brief 時間逆転ダメージを計算する (耐性があれば、(1d4 + 4) / 9になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_time(player_type *target_ptr, monap_type *monap_ptr)
{
    if (monap_ptr->explode)
        return;

    process_monster_attack_time(target_ptr, monap_ptr);
    if (has_resist_time(target_ptr))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
}

/*!
 * @brief 生命力吸収ダメージを計算する (経験値維持があれば9/10になる)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_drain_life(player_type *target_ptr, monap_type *monap_ptr)
{
    s32b d = damroll(60, 6) + (target_ptr->exp / 100) * MON_DRAIN_LIFE;
    monap_ptr->obvious = TRUE;
    if (target_ptr->hold_exp)
        monap_ptr->damage = monap_ptr->damage * 9 / 10;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    bool resist_drain = check_drain_hp(target_ptr, d);
    process_drain_life(target_ptr, monap_ptr, resist_drain);
}

/*!
 * @brief MPダメージを計算する (消費魔力減少、呪文失敗率減少、魔道具使用能力向上があればそれぞれ-5%)
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレーヤーへの直接攻撃構造体への参照ポインタ
 * @return なし
 */
static void calc_blow_drain_mana(player_type *target_ptr, monap_type *monap_ptr)
{
    monap_ptr->obvious = TRUE;
    int damage_ratio = 100;
    if (has_dec_mana(target_ptr))
        damage_ratio -= 5;

    if (has_easy_spell(target_ptr))
        damage_ratio -= 5;

    if (has_magic_mastery(target_ptr))
        damage_ratio -= 5;

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 100;
    process_drain_mana(target_ptr, monap_ptr);
    update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_MANA);
}

static void calc_blow_inertia(player_type *target_ptr, monap_type *monap_ptr)
{
    if ((target_ptr->fast > 0) || (target_ptr->pspeed >= 130))
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;

    monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
    if (target_ptr->is_dead || check_multishadow(target_ptr))
        return;

    if (set_slow(target_ptr, (target_ptr->slow + 4 + randint0(monap_ptr->rlev / 10)), FALSE))
        monap_ptr->obvious = TRUE;
}

void switch_monster_blow_to_player(player_type *target_ptr, monap_type *monap_ptr)
{
    switch (monap_ptr->effect) {
    case RBE_NONE:
        // ここには来ないはずだが、何らかのバグで来た場合はプレイヤーの不利益に
        // ならないようダメージを 0 にしておく。
        monap_ptr->damage = 0;
        break;
    case RBE_SUPERHURT: { /* AC軽減あり / Player armor reduces total damage */
        if (((randint1(monap_ptr->rlev * 2 + 300) > (monap_ptr->ac + 200)) || one_in_(13)) && !check_multishadow(target_ptr)) {
            int tmp_damage = monap_ptr->damage - (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
            msg_print(_("痛恨の一撃！", "It was a critical hit!"));
            tmp_damage = MAX(monap_ptr->damage, tmp_damage * 2);
            monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, tmp_damage, monap_ptr->ddesc, -1);
            break;
        }
    }
        /* Fall through */
    case RBE_HURT: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = TRUE;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        break;
    }
    case RBE_POISON:
        calc_blow_poison(target_ptr, monap_ptr);
        break;
    case RBE_UN_BONUS:
        calc_blow_disenchant(target_ptr, monap_ptr);
        break;
    case RBE_UN_POWER:
        calc_blow_un_power(target_ptr, monap_ptr);
        break;
    case RBE_EAT_GOLD:
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (monster_confused_remaining(monap_ptr->m_ptr) || target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        monap_ptr->obvious = TRUE;
        process_eat_gold(target_ptr, monap_ptr);
        break;
    case RBE_EAT_ITEM: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (!check_eat_item(target_ptr, monap_ptr))
            break;

        process_eat_item(target_ptr, monap_ptr);
        break;
    }

    case RBE_EAT_FOOD: {
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        process_eat_food(target_ptr, monap_ptr);
        break;
    }
    case RBE_EAT_LITE: {
        monap_ptr->o_ptr = &target_ptr->inventory_list[INVEN_LITE];
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead || check_multishadow(target_ptr))
            break;

        process_eat_lite(target_ptr, monap_ptr);
        break;
    }
    case RBE_ACID: {
        if (monap_ptr->explode)
            break;

        monap_ptr->obvious = TRUE;
        msg_print(_("酸を浴びせられた！", "You are covered in acid!"));
        monap_ptr->get_damage += acid_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
        update_creature(target_ptr);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_ACID);
        break;
    }
    case RBE_ELEC: {
        if (monap_ptr->explode)
            break;

        monap_ptr->obvious = TRUE;
        msg_print(_("電撃を浴びせられた！", "You are struck by electricity!"));
        monap_ptr->get_damage += elec_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_ELEC);
        break;
    }
    case RBE_FIRE: {
        if (monap_ptr->explode)
            break;

        monap_ptr->obvious = TRUE;
        msg_print(_("全身が炎に包まれた！", "You are enveloped in flames!"));
        monap_ptr->get_damage += fire_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_FIRE);
        break;
    }
    case RBE_COLD: {
        if (monap_ptr->explode)
            break;

        monap_ptr->obvious = TRUE;
        msg_print(_("全身が冷気で覆われた！", "You are covered with frost!"));
        monap_ptr->get_damage += cold_dam(target_ptr, monap_ptr->damage, monap_ptr->ddesc, -1, FALSE);
        update_smart_learn(target_ptr, monap_ptr->m_idx, DRS_COLD);
        break;
    }
    case RBE_BLIND:
        calc_blow_blind(target_ptr, monap_ptr);
        break;
    case RBE_CONFUSE:
        calc_blow_confusion(target_ptr, monap_ptr);
        break;
    case RBE_TERRIFY:
        calc_blow_fear(target_ptr, monap_ptr);
        break;
    case RBE_PARALYZE:
        calc_blow_paralysis(target_ptr, monap_ptr);
        break;
    case RBE_LOSE_STR:
        calc_blow_lose_strength(target_ptr, monap_ptr);
        break;
    case RBE_LOSE_INT:
        calc_blow_lose_intelligence(target_ptr, monap_ptr);
        break;
    case RBE_LOSE_WIS:
        calc_blow_lose_wisdom(target_ptr, monap_ptr);
        break;
    case RBE_LOSE_DEX:
        calc_blow_lose_dexterity(target_ptr, monap_ptr);
        break;
    case RBE_LOSE_CON:
        calc_blow_lose_constitution(target_ptr, monap_ptr);
        break;
    case RBE_LOSE_CHR:
        calc_blow_lose_charisma(target_ptr, monap_ptr);
        break;
    case RBE_LOSE_ALL:
        calc_blow_lose_all(target_ptr, monap_ptr);
        break;
    case RBE_SHATTER: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = TRUE;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (monap_ptr->damage > 23 || monap_ptr->explode)
            earthquake(target_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, 8, monap_ptr->m_idx);

        break;
    }
    case RBE_EXP_10:
        calc_blow_drain_exp(target_ptr, monap_ptr, 10, 95);
        break;
    case RBE_EXP_20:
        calc_blow_drain_exp(target_ptr, monap_ptr, 20, 90);
        break;
    case RBE_EXP_40:
        calc_blow_drain_exp(target_ptr, monap_ptr, 40, 75);
        break;
    case RBE_EXP_80:
        calc_blow_drain_exp(target_ptr, monap_ptr, 80, 50);
        break;
    case RBE_DISEASE:
        calc_blow_disease(target_ptr, monap_ptr);
        break;
    case RBE_TIME:
        calc_blow_time(target_ptr, monap_ptr);
        break;
    case RBE_DR_LIFE:
        calc_blow_drain_life(target_ptr, monap_ptr);
        break;
    case RBE_DR_MANA:
        calc_blow_drain_mana(target_ptr, monap_ptr);
        break;
    case RBE_INERTIA:
        calc_blow_inertia(target_ptr, monap_ptr);
        break;
    case RBE_STUN:
        monap_ptr->get_damage += take_hit(target_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc, -1);
        if (target_ptr->is_dead)
            break;

        process_stun_attack(target_ptr, monap_ptr);
        break;
    case RBE_FLAVOR:
        // フレーバー打撃は自明かつダメージ 0。
        monap_ptr->obvious = TRUE;
        monap_ptr->damage = 0;
        break;
    }
}
