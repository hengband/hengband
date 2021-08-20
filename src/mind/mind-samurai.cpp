/*!
 * @brief 剣術家のレイシャルパワー処理
 * @date 2020/05/16
 * @author Hourier
 */

#include "mind/mind-samurai.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-action/cmd-attack.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "mind/stances-table.h"
#include "monster-attack/monster-attack-util.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-resistance.h"
#include "monster-race/race-flags3.h"
#include "monster/monster-describer.h"
#include "monster/monster-info.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "object-enchant/tr-types.h"
#include "pet/pet-util.h"
#include "player-attack/player-attack-util.h"
#include "player/attack-defense-types.h"
#include "status/action-setter.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

typedef struct samurai_slaying_type {
    MULTIPLY mult;
    TrFlags flags;
    monster_type *m_ptr;
    combat_options mode;
    monster_race *r_ptr;
} samurai_slaying_type;

static samurai_slaying_type *initialize_samurai_slaying_type(
    samurai_slaying_type *samurai_slaying_ptr, MULTIPLY mult, const TrFlags &flags, monster_type *m_ptr, combat_options mode, monster_race *r_ptr)
{
    samurai_slaying_ptr->mult = mult;
    std::copy_n(flags, TR_FLAG_SIZE, samurai_slaying_ptr->flags);
    samurai_slaying_ptr->m_ptr = m_ptr;
    samurai_slaying_ptr->mode = mode;
    samurai_slaying_ptr->r_ptr = r_ptr;
    return samurai_slaying_ptr;
}

/*!
 * @nrief 焔霊 (焼棄スレイ)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_burning_strike(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_FIRE)
        return;

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
            samurai_slaying_ptr->r_ptr->r_flagsr |= (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);

        return;
    }

    /* Otherwise, take the damage */
    if (has_flag(samurai_slaying_ptr->flags, TR_BRAND_FIRE)) {
        if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_FIRE) {
            if (samurai_slaying_ptr->mult < 70)
                samurai_slaying_ptr->mult = 70;

            if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
                samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_FIRE;

        } else if (samurai_slaying_ptr->mult < 35)
            samurai_slaying_ptr->mult = 35;

        return;
    }

    if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_FIRE) {
        if (samurai_slaying_ptr->mult < 50)
            samurai_slaying_ptr->mult = 50;

        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
            samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_FIRE;
    } else if (samurai_slaying_ptr->mult < 25)
        samurai_slaying_ptr->mult = 25;
}

/*!
 * @brief サーペンツタン (毒殺スレイ)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_serpent_tongue(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_POISON)
        return;

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_POIS_MASK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
            samurai_slaying_ptr->r_ptr->r_flagsr |= (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_POIS_MASK);

        return;
    }

    /* Otherwise, take the damage */
    if (has_flag(samurai_slaying_ptr->flags, TR_BRAND_POIS)) {
        if (samurai_slaying_ptr->mult < 35)
            samurai_slaying_ptr->mult = 35;
    } else if (samurai_slaying_ptr->mult < 25)
        samurai_slaying_ptr->mult = 25;
}

/*!
 * @brief 二重の極み^h^h^h^h^h 斬魔剣弐の太刀 (邪悪無生命スレイ)
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_zanma_ken(samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_ZANMA)
        return;

    if (!monster_living(samurai_slaying_ptr->m_ptr->r_idx) && (samurai_slaying_ptr->r_ptr->flags3 & RF3_EVIL)) {
        if (samurai_slaying_ptr->mult < 15)
            samurai_slaying_ptr->mult = 25;
        else if (samurai_slaying_ptr->mult < 50)
            samurai_slaying_ptr->mult = MIN(50, samurai_slaying_ptr->mult + 20);
    }
}

/*!
 * @brief 破岩斬 (岩石スレイ)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_rock_smash(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_HAGAN)
        return;

    if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_ROCK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
            samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_ROCK;

        if (samurai_slaying_ptr->mult == 10)
            samurai_slaying_ptr->mult = 40;
        else if (samurai_slaying_ptr->mult < 60)
            samurai_slaying_ptr->mult = 60;
    }
}

/*!
 * @brief 乱れ雪月花 (冷気スレイ)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_midare_setsugetsuka(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_COLD)
        return;

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_COLD_MASK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
            samurai_slaying_ptr->r_ptr->r_flagsr |= (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);

        return;
    }

    /* Otherwise, take the damage */
    if (has_flag(samurai_slaying_ptr->flags, TR_BRAND_COLD)) {
        if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_COLD) {
            if (samurai_slaying_ptr->mult < 70)
                samurai_slaying_ptr->mult = 70;

            if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
                samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_COLD;
        } else if (samurai_slaying_ptr->mult < 35)
            samurai_slaying_ptr->mult = 35;

        return;
    }

    if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_COLD) {
        if (samurai_slaying_ptr->mult < 50)
            samurai_slaying_ptr->mult = 50;

        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
            samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_COLD;
    } else if (samurai_slaying_ptr->mult < 25)
        samurai_slaying_ptr->mult = 25;
}

/*!
 * @brief 雷撃鷲爪斬
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_lightning_eagle(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_ELEC)
        return;

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr))
            samurai_slaying_ptr->r_ptr->r_flagsr |= (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);

        return;
    }

    /* Otherwise, take the damage */
    if (has_flag(samurai_slaying_ptr->flags, TR_BRAND_ELEC)) {
        if (samurai_slaying_ptr->mult < 70)
            samurai_slaying_ptr->mult = 70;
    } else if (samurai_slaying_ptr->mult < 50)
        samurai_slaying_ptr->mult = 50;
}

/*!
 * @brief 赤流渦 (ペインバッカー)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_bloody_maelstroem(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if ((samurai_slaying_ptr->mode == HISSATSU_SEKIRYUKA) && attacker_ptr->cut && monster_living(samurai_slaying_ptr->m_ptr->r_idx)) {
        MULTIPLY tmp = MIN(100, MAX(10, attacker_ptr->cut / 10));
        if (samurai_slaying_ptr->mult < tmp)
            samurai_slaying_ptr->mult = tmp;
    }
}

/*!
 * @brief 慶雲鬼忍剣 (アンデッドスレイ)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 */
static void hissatsu_keiun_kininken(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_UNDEAD)
        return;

    if (samurai_slaying_ptr->r_ptr->flags3 & RF3_UNDEAD)
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_UNDEAD;

            if (samurai_slaying_ptr->mult == 10)
                samurai_slaying_ptr->mult = 70;
            else if (samurai_slaying_ptr->mult < 140)
                samurai_slaying_ptr->mult = MIN(140, samurai_slaying_ptr->mult + 60);
        }

    if (samurai_slaying_ptr->mult == 10)
        samurai_slaying_ptr->mult = 40;
    else if (samurai_slaying_ptr->mult < 60)
        samurai_slaying_ptr->mult = MIN(60, samurai_slaying_ptr->mult + 30);
}

/*!
 * @brief 剣術のスレイ倍率計算を行う /
 * Calcurate magnification of hissatsu technics
 * @param mult 剣術のスレイ効果以前に算出している多要素の倍率(/10倍)
 * @param flags 剣術に使用する武器のスレイフラグ配列
 * @param m_ptr 目標となるモンスターの構造体参照ポインタ
 * @param mode 剣術のスレイ型ID
 * @return スレイの倍率(/10倍)
 */
MULTIPLY mult_hissatsu(player_type *attacker_ptr, MULTIPLY mult, const TrFlags &flags, monster_type *m_ptr, combat_options mode)
{
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    samurai_slaying_type tmp_slaying;
    samurai_slaying_type *samurai_slaying_ptr = initialize_samurai_slaying_type(&tmp_slaying, mult, flags, m_ptr, mode, r_ptr);
    hissatsu_burning_strike(attacker_ptr, samurai_slaying_ptr);
    hissatsu_serpent_tongue(attacker_ptr, samurai_slaying_ptr);
    hissatsu_zanma_ken(samurai_slaying_ptr);
    hissatsu_rock_smash(attacker_ptr, samurai_slaying_ptr);
    hissatsu_midare_setsugetsuka(attacker_ptr, samurai_slaying_ptr);
    hissatsu_lightning_eagle(attacker_ptr, samurai_slaying_ptr);
    hissatsu_bloody_maelstroem(attacker_ptr, samurai_slaying_ptr);
    hissatsu_keiun_kininken(attacker_ptr, samurai_slaying_ptr);

    if (samurai_slaying_ptr->mult > 150)
        samurai_slaying_ptr->mult = 150;

    return samurai_slaying_ptr->mult;
}

void concentration(player_type *creature_ptr)
{
    int max_csp = MAX(creature_ptr->msp * 4, creature_ptr->lev * 5 + 5);

    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return;
    }

    if (creature_ptr->special_defense & KATA_MASK) {
        msg_print(_("今は構えに集中している。", "You're already concentrating on your stance."));
        return;
    }

    msg_print(_("精神を集中して気合いを溜めた。", "You concentrate to charge your power."));

    creature_ptr->csp += creature_ptr->msp / 2;
    if (creature_ptr->csp >= max_csp) {
        creature_ptr->csp = max_csp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= PR_MANA;
}

/*!
 * @brief 剣術家の型設定処理
 * @return 型を変化させたらTRUE、型の構え不能かキャンセルしたらFALSEを返す。
 */
bool choose_kata(player_type *creature_ptr)
{
    char choice;
    int new_kata = 0;
    int i;
    char buf[80];

    if (cmd_limit_confused(creature_ptr))
        return false;

    if (creature_ptr->stun) {
        msg_print(_("意識がはっきりとしない。", "You are not clear-headed"));
        return false;
    }

    if (creature_ptr->afraid) {
        msg_print(_("体が震えて構えられない！", "You are trembling with fear!"));
        return false;
    }

    screen_save();
    prt(_(" a) 型を崩す", " a) No Form"), 2, 20);
    for (i = 0; i < MAX_KATA; i++) {
        if (creature_ptr->lev >= samurai_stances[i].min_level) {
            sprintf(buf, _(" %c) %sの型    %s", " %c) Stance of %-12s  %s"), I2A(i + 1), samurai_stances[i].desc, samurai_stances[i].info);
            prt(buf, 3 + i, 20);
        }
    }

    prt("", 1, 0);
    prt(_("        どの型で構えますか？", "        Choose Stance: "), 1, 14);

    while (true) {
        choice = inkey();

        if (choice == ESCAPE) {
            screen_load();
            return false;
        } else if ((choice == 'a') || (choice == 'A')) {
            if (creature_ptr->action == ACTION_KATA) {
                set_action(creature_ptr, ACTION_NONE);
            } else
                msg_print(_("もともと構えていない。", "You are not in a special stance."));
            screen_load();
            return true;
        } else if ((choice == 'b') || (choice == 'B')) {
            new_kata = 0;
            break;
        } else if (((choice == 'c') || (choice == 'C')) && (creature_ptr->lev > 29)) {
            new_kata = 1;
            break;
        } else if (((choice == 'd') || (choice == 'D')) && (creature_ptr->lev > 34)) {
            new_kata = 2;
            break;
        } else if (((choice == 'e') || (choice == 'E')) && (creature_ptr->lev > 39)) {
            new_kata = 3;
            break;
        }
    }

    set_action(creature_ptr, ACTION_KATA);
    if (creature_ptr->special_defense & (KATA_IAI << new_kata)) {
        msg_print(_("構え直した。", "You reassume a stance."));
    } else {
        creature_ptr->special_defense &= ~(KATA_MASK);
        creature_ptr->update |= (PU_BONUS | PU_MONSTERS);
        msg_format(_("%sの型で構えた。", "You assume the %s stance."), samurai_stances[new_kata].desc);
        creature_ptr->special_defense |= (KATA_IAI << new_kata);
    }

    creature_ptr->redraw |= (PR_STATE | PR_STATUS);
    screen_load();
    return true;
}

/*!
 * @brief 剣術家限定で、型等に応じて命中率を高める
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 * @return 上昇後の命中率
 */
int calc_attack_quality(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    object_type *o_ptr = &attacker_ptr->inventory_list[INVEN_MAIN_HAND + pa_ptr->hand];
    int bonus = attacker_ptr->to_h[pa_ptr->hand] + o_ptr->to_h;
    int chance = (attacker_ptr->skill_thn + (bonus * BTH_PLUS_ADJ));
    if (pa_ptr->mode == HISSATSU_IAI)
        chance += 60;

    if (attacker_ptr->special_defense & KATA_KOUKIJIN)
        chance += 150;

    if (attacker_ptr->sutemi)
        chance = MAX(chance * 3 / 2, chance + 60);

    int vir = virtue_number(attacker_ptr, V_VALOUR);
    if (vir != 0)
        chance += (attacker_ptr->virtues[vir - 1] / 10);

    return chance;
}

/*!
 * @brief 峰打ちの効果処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void mineuchi(player_type *attacker_ptr, player_attack_type *pa_ptr)
{
    if (pa_ptr->mode != HISSATSU_MINEUCHI)
        return;

    pa_ptr->attack_damage = 0;
    anger_monster(attacker_ptr, pa_ptr->m_ptr);

    monster_race *r_ptr = &r_info[pa_ptr->m_ptr->r_idx];
    if ((r_ptr->flags3 & (RF3_NO_STUN))) {
        msg_format(_("%s には効果がなかった。", "%s is not effected."), pa_ptr->m_name);
        return;
    }

    int tmp = (10 + randint1(15) + attacker_ptr->lev / 5);
    if (monster_stunned_remaining(pa_ptr->m_ptr)) {
        msg_format(_("%sはひどくもうろうとした。", "%s is more dazed."), pa_ptr->m_name);
        tmp /= 2;
    } else
        msg_format(_("%s はもうろうとした。", "%s is dazed."), pa_ptr->m_name);

    (void)set_monster_stunned(attacker_ptr, pa_ptr->g_ptr->m_idx, monster_stunned_remaining(pa_ptr->m_ptr) + tmp);
}

/*!
 * @brief 無想による反撃処理
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param pa_ptr 直接攻撃構造体への参照ポインタ
 */
void musou_counterattack(player_type *attacker_ptr, monap_type *monap_ptr)
{
    if ((!attacker_ptr->counter && ((attacker_ptr->special_defense & KATA_MUSOU) == 0)) || !monap_ptr->alive || attacker_ptr->is_dead || !monap_ptr->m_ptr->ml
        || (attacker_ptr->csp <= 7))
        return;

    char m_target_name[MAX_NLEN];
    monster_desc(attacker_ptr, m_target_name, monap_ptr->m_ptr, 0);
    attacker_ptr->csp -= 7;
    msg_format(_("%^sに反撃した！", "You counterattacked %s!"), m_target_name);
    do_cmd_attack(attacker_ptr, monap_ptr->m_ptr->fy, monap_ptr->m_ptr->fx, HISSATSU_COUNTER);
    monap_ptr->fear = false;
    attacker_ptr->redraw |= (PR_MANA);
}
