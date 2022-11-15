/*!
 * @brief モンスターからプレイヤーへの直接攻撃をその種別において振り分ける
 * @date 2020/05/31
 * @author Hourier
 * @details 長い処理はインクルード先の別ファイルにて行っている
 */

#include "monster-attack/monster-attack-switcher.h"
#include "inventory/inventory-slot-types.h"
#include "mind/drs-types.h"
#include "mind/mind-mirror-master.h"
#include "monster-attack/monster-attack-lose.h"
#include "monster-attack/monster-attack-player.h"
#include "monster-attack/monster-attack-status.h"
#include "monster-attack/monster-attack-table.h"
#include "monster-attack/monster-eating.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "player/player-status-resist.h"
#include "player/player-status.h"
#include "spell-kind/earthquake.h"
#include "spell-kind/spells-equipment.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "status/element-resistance.h"
#include "status/experience.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-acceleration.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"

/*!
 * @brief 毒ダメージを計算する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 * @details 減衰の計算式がpoisではなくnukeなのは仕様 (1/3では減衰が強すぎると判断したため)
 */
static void calc_blow_poison(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->explode) {
        return;
    }

    if (!(has_resist_pois(player_ptr) || is_oppose_pois(player_ptr)) && !check_multishadow(player_ptr) && BadStatusSetter(player_ptr).mod_poison(randint1(monap_ptr->rlev) + 5)) {
        monap_ptr->obvious = true;
    }

    monap_ptr->damage = monap_ptr->damage * calc_nuke_damage_rate(player_ptr) / 100;
    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_POIS);
}

/*!
 * @brief 劣化ダメージを計算する (耐性があれば、(1d4 + 4) / 9になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_disenchant(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->explode) {
        return;
    }

    if (!has_resist_disen(player_ptr) && !check_multishadow(player_ptr) && apply_disenchant(player_ptr, 0)) {
        update_creature(player_ptr);
        monap_ptr->obvious = true;
    }

    if (has_resist_disen(player_ptr)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_DISEN);
}

/*!
 * @brief 魔道具吸収ダメージを計算する (消費魔力減少、呪文失敗率減少、魔道具使用能力向上があればそれぞれ-7.5%)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 * @detals 魔道具使用能力向上フラグがあれば、吸収対象のアイテムをスキャンされる回数が半分で済む
 */
static void calc_blow_un_power(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    int damage_ratio = 1000;
    if (has_dec_mana(player_ptr)) {
        damage_ratio -= 75;
    }

    if (has_easy_spell(player_ptr)) {
        damage_ratio -= 75;
    }

    bool is_magic_mastery = has_magic_mastery(player_ptr) != 0;
    if (is_magic_mastery) {
        damage_ratio -= 75;
    }

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 1000;
    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (player_ptr->is_dead || check_multishadow(player_ptr)) {
        return;
    }

    int max_draining_item = is_magic_mastery ? 5 : 10;
    for (int i = 0; i < max_draining_item; i++) {
        INVENTORY_IDX i_idx = (INVENTORY_IDX)randint0(INVEN_PACK);
        monap_ptr->o_ptr = &player_ptr->inventory_list[i_idx];
        if (monap_ptr->o_ptr->k_idx == 0) {
            continue;
        }

        if (process_un_power(player_ptr, monap_ptr)) {
            break;
        }
    }
}

/*!
 * @brief 盲目ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_blind(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_blind(player_ptr)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;
    }

    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (player_ptr->is_dead) {
        return;
    }

    process_blind_attack(player_ptr, monap_ptr);
    update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_BLIND);
}

/*!
 * @brief 混乱ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_confusion(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->explode) {
        return;
    }

    if (has_resist_conf(player_ptr)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;
    }

    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (player_ptr->is_dead) {
        return;
    }

    if (!has_resist_conf(player_ptr) && !check_multishadow(player_ptr) && BadStatusSetter(player_ptr).mod_confusion(3 + randint1(monap_ptr->rlev))) {
        monap_ptr->obvious = true;
    }

    update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_CONF);
}

/*!
 * @brief 恐怖ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_fear(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (has_resist_fear(player_ptr)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;
    }

    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (player_ptr->is_dead) {
        return;
    }

    process_terrify_attack(player_ptr, monap_ptr);
    update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_FEAR);
}

/*!
 * @brief 麻痺ダメージを計算する (耐性があれば、(1d4 + 3) / 8になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_paralysis(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (has_free_act(player_ptr)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 3) / 8;
    }

    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (player_ptr->is_dead) {
        return;
    }

    process_paralyze_attack(player_ptr, monap_ptr);
    update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_FREE);
}

/*!
 * @brief 経験値吸収ダメージを計算する (経験値保持と地獄耐性があれば、それぞれ-7.5%)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_drain_exp(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr, const int drain_value, const int hold_exp_prob)
{
    int32_t d = damroll(drain_value, 6) + (player_ptr->exp / 100) * MON_DRAIN_LIFE;
    monap_ptr->obvious = true;
    int damage_ratio = 1000;
    if (has_hold_exp(player_ptr)) {
        damage_ratio -= 75;
    }

    if (has_resist_neth(player_ptr)) {
        damage_ratio -= 75;
    }

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 1000;
    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (player_ptr->is_dead || check_multishadow(player_ptr)) {
        return;
    }

    (void)drain_exp(player_ptr, d, d / 10, hold_exp_prob);
}

/*!
 * @brief 時間逆転ダメージを計算する (耐性があれば、(1d4 + 4) / 9になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_time(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (monap_ptr->explode) {
        return;
    }

    process_monster_attack_time(player_ptr, monap_ptr);
    if (has_resist_time(player_ptr)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
}

/*!
 * @brief 生命力吸収ダメージを計算する (経験値維持があれば9/10になる)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_drain_life(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    int32_t d = damroll(60, 6) + (player_ptr->exp / 100) * MON_DRAIN_LIFE;
    monap_ptr->obvious = true;
    if (player_ptr->hold_exp) {
        monap_ptr->damage = monap_ptr->damage * 9 / 10;
    }

    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (player_ptr->is_dead || check_multishadow(player_ptr)) {
        return;
    }

    bool resist_drain = check_drain_hp(player_ptr, d);
    process_drain_life(player_ptr, monap_ptr, resist_drain);
}

/*!
 * @brief MPダメージを計算する (消費魔力減少、呪文失敗率減少、魔道具使用能力向上があればそれぞれ-5%)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param monap_ptr モンスターからプレイヤーへの直接攻撃構造体への参照ポインタ
 */
static void calc_blow_drain_mana(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    monap_ptr->obvious = true;
    int damage_ratio = 100;
    if (has_dec_mana(player_ptr)) {
        damage_ratio -= 5;
    }

    if (has_easy_spell(player_ptr)) {
        damage_ratio -= 5;
    }

    if (has_magic_mastery(player_ptr)) {
        damage_ratio -= 5;
    }

    monap_ptr->damage = monap_ptr->damage * damage_ratio / 100;
    process_drain_mana(player_ptr, monap_ptr);
    update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_MANA);
}

static void calc_blow_inertia(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (player_ptr->effects()->acceleration()->is_fast() || (player_ptr->pspeed >= 130)) {
        monap_ptr->damage = monap_ptr->damage * (randint1(4) + 4) / 9;
    }

    monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
    if (player_ptr->is_dead || check_multishadow(player_ptr)) {
        return;
    }

    if (BadStatusSetter(player_ptr).mod_deceleration(4 + randint0(monap_ptr->rlev / 10), false)) {
        monap_ptr->obvious = true;
    }
}

/*!
 * @brief 空腹進行度を計算する (急速回復があれば+100%、遅消化があれば-50%)
 */
static void calc_blow_hungry(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    if (player_ptr->regenerate) {
        monap_ptr->damage = monap_ptr->damage * 2;
    }
    if (player_ptr->slow_digest) {
        monap_ptr->damage = monap_ptr->damage / 2;
    }

    process_monster_attack_hungry(player_ptr, monap_ptr);
}

void switch_monster_blow_to_player(PlayerType *player_ptr, MonsterAttackPlayer *monap_ptr)
{
    switch (monap_ptr->effect) {
    case RaceBlowEffectType::NONE:
        // ここには来ないはずだが、何らかのバグで来た場合はプレイヤーの不利益に
        // ならないようダメージを 0 にしておく。
        monap_ptr->damage = 0;
        break;
    case RaceBlowEffectType::SUPERHURT: { /* AC軽減あり / Player armor reduces total damage */
        if (((randint1(monap_ptr->rlev * 2 + 300) > (monap_ptr->ac + 200)) || one_in_(13)) && !check_multishadow(player_ptr)) {
            monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
            msg_print(_("痛恨の一撃！", "It was a critical hit!"));
            monap_ptr->damage = std::max(monap_ptr->damage, monap_ptr->damage * 2);
            monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
            break;
        }
    }
        [[fallthrough]];
    case RaceBlowEffectType::HURT: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = true;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
        break;
    }
    case RaceBlowEffectType::POISON:
        calc_blow_poison(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::UN_BONUS:
        calc_blow_disenchant(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::UN_POWER:
        calc_blow_un_power(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::EAT_GOLD:
        monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
        if (monap_ptr->m_ptr->is_confused() || player_ptr->is_dead || check_multishadow(player_ptr)) {
            break;
        }

        monap_ptr->obvious = true;
        process_eat_gold(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::EAT_ITEM: {
        monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
        if (!check_eat_item(player_ptr, monap_ptr)) {
            break;
        }

        process_eat_item(player_ptr, monap_ptr);
        break;
    }

    case RaceBlowEffectType::EAT_FOOD: {
        monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
        if (player_ptr->is_dead || check_multishadow(player_ptr)) {
            break;
        }

        process_eat_food(player_ptr, monap_ptr);
        break;
    }
    case RaceBlowEffectType::EAT_LITE: {
        monap_ptr->o_ptr = &player_ptr->inventory_list[INVEN_LITE];
        monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
        if (player_ptr->is_dead || check_multishadow(player_ptr)) {
            break;
        }

        process_eat_lite(player_ptr, monap_ptr);
        break;
    }
    case RaceBlowEffectType::ACID: {
        if (monap_ptr->explode) {
            break;
        }

        monap_ptr->obvious = true;
        msg_print(_("酸を浴びせられた！", "You are covered in acid!"));
        monap_ptr->get_damage += acid_dam(player_ptr, monap_ptr->damage, monap_ptr->ddesc, false);
        update_creature(player_ptr);
        update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_ACID);
        break;
    }
    case RaceBlowEffectType::ELEC: {
        if (monap_ptr->explode) {
            break;
        }

        monap_ptr->obvious = true;
        msg_print(_("電撃を浴びせられた！", "You are struck by electricity!"));
        monap_ptr->get_damage += elec_dam(player_ptr, monap_ptr->damage, monap_ptr->ddesc, false);
        update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_ELEC);
        break;
    }
    case RaceBlowEffectType::FIRE: {
        if (monap_ptr->explode) {
            break;
        }

        monap_ptr->obvious = true;
        msg_print(_("全身が炎に包まれた！", "You are enveloped in flames!"));
        monap_ptr->get_damage += fire_dam(player_ptr, monap_ptr->damage, monap_ptr->ddesc, false);
        update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_FIRE);
        break;
    }
    case RaceBlowEffectType::COLD: {
        if (monap_ptr->explode) {
            break;
        }

        monap_ptr->obvious = true;
        msg_print(_("全身が冷気で覆われた！", "You are covered with frost!"));
        monap_ptr->get_damage += cold_dam(player_ptr, monap_ptr->damage, monap_ptr->ddesc, false);
        update_smart_learn(player_ptr, monap_ptr->m_idx, DRS_COLD);
        break;
    }
    case RaceBlowEffectType::BLIND:
        calc_blow_blind(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::CONFUSE:
        calc_blow_confusion(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::TERRIFY:
        calc_blow_fear(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::PARALYZE:
        calc_blow_paralysis(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_STR:
        calc_blow_lose_strength(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_INT:
        calc_blow_lose_intelligence(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_WIS:
        calc_blow_lose_wisdom(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_DEX:
        calc_blow_lose_dexterity(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_CON:
        calc_blow_lose_constitution(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_CHR:
        calc_blow_lose_charisma(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::LOSE_ALL:
        calc_blow_lose_all(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::SHATTER: { /* AC軽減あり / Player armor reduces total damage */
        monap_ptr->obvious = true;
        monap_ptr->damage -= (monap_ptr->damage * ((monap_ptr->ac < 150) ? monap_ptr->ac : 150) / 250);
        monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
        if (monap_ptr->damage > 23 || monap_ptr->explode) {
            earthquake(player_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, 8, monap_ptr->m_idx);
        }

        break;
    }
    case RaceBlowEffectType::EXP_10:
        calc_blow_drain_exp(player_ptr, monap_ptr, 10, 95);
        break;
    case RaceBlowEffectType::EXP_20:
        calc_blow_drain_exp(player_ptr, monap_ptr, 20, 90);
        break;
    case RaceBlowEffectType::EXP_40:
        calc_blow_drain_exp(player_ptr, monap_ptr, 40, 75);
        break;
    case RaceBlowEffectType::EXP_80:
        calc_blow_drain_exp(player_ptr, monap_ptr, 80, 50);
        break;
    case RaceBlowEffectType::DISEASE:
        calc_blow_disease(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::TIME:
        calc_blow_time(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::DR_LIFE:
        calc_blow_drain_life(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::DR_MANA:
        calc_blow_drain_mana(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::INERTIA:
        calc_blow_inertia(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::STUN:
        monap_ptr->get_damage += take_hit(player_ptr, DAMAGE_ATTACK, monap_ptr->damage, monap_ptr->ddesc);
        if (player_ptr->is_dead) {
            break;
        }
        process_stun_attack(player_ptr, monap_ptr);
        break;
    case RaceBlowEffectType::FLAVOR:
        // フレーバー打撃は自明かつダメージ 0。
        monap_ptr->obvious = true;
        monap_ptr->damage = 0;
        break;
    case RaceBlowEffectType::HUNGRY:
        calc_blow_hungry(player_ptr, monap_ptr);
        break;

    case RaceBlowEffectType::MAX:
        break;
    }
}
