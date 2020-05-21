/*!
 * @brief 剣術家の属性スレイ処理
 * @date 2020/05/21
 * @author Hourier
 */

#include "system/angband.h"
#include "mind/samurai-slaying.h"
#include "monster/monsterrace-hook.h"

typedef struct samurai_slaying_type {
    MULTIPLY mult;
    BIT_FLAGS *flags;
    monster_type *m_ptr;
    combat_options mode;
    monster_race *r_ptr;
} samurai_slaying_type;

static samurai_slaying_type *initialize_samurai_slaying_type(samurai_slaying_type *samurai_slaying_ptr, MULTIPLY mult, BIT_FLAGS *flags, monster_type *m_ptr, combat_options mode, monster_race *r_ptr)
{
    samurai_slaying_ptr->mult = mult;
    samurai_slaying_ptr->flags = flags;
    samurai_slaying_ptr->m_ptr = m_ptr;
    samurai_slaying_ptr->mode = mode;
    samurai_slaying_ptr->r_ptr = r_ptr;
    return samurai_slaying_ptr;
}

/*!
 * @nrief 焔霊 (焼棄スレイ)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 * @return なし
 */
static void hissatsu_burning_strike(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_FIRE)
        return;

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flagsr |= (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_FIRE_MASK);
        }

        return;
    }

    /* Otherwise, take the damage */
    if (have_flag(samurai_slaying_ptr->flags, TR_BRAND_FIRE)) {
        if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_FIRE) {
            if (samurai_slaying_ptr->mult < 70)
                samurai_slaying_ptr->mult = 70;
            if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
                samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_FIRE;
            }
        } else if (samurai_slaying_ptr->mult < 35)
            samurai_slaying_ptr->mult = 35;

        return;
    }

    if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_FIRE) {
        if (samurai_slaying_ptr->mult < 50)
            samurai_slaying_ptr->mult = 50;
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_FIRE;
        }
    } else if (samurai_slaying_ptr->mult < 25)
        samurai_slaying_ptr->mult = 25;
}

/*!
 * @brief サーペンツタン (毒殺スレイ)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 * @return なし
 */
static void hissatsu_serpent_tongue(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_POISON)
        return;

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_POIS_MASK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flagsr |= (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_POIS_MASK);
        }

        return;
    }

    /* Otherwise, take the damage */
    if (have_flag(samurai_slaying_ptr->flags, TR_BRAND_POIS)) {
        if (samurai_slaying_ptr->mult < 35)
            samurai_slaying_ptr->mult = 35;
    } else {
        if (samurai_slaying_ptr->mult < 25)
            samurai_slaying_ptr->mult = 25;
    }
}

/*!
 * @brief 二重の極み^h^h^h^h^h 斬魔剣弐の太刀 (邪悪無生命スレイ)
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 * @return なし
 */
static void hissatsu_zanma_ken(samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode == HISSATSU_ZANMA)
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
 * @return なし
 */
static void hissatsu_rock_smash(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_HAGAN)
        return;

    if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_ROCK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_ROCK;
        }
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
 * @return なし
 */
static void hissatsu_midare_setsugetsuka(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_COLD)
        return;

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_COLD_MASK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flagsr |= (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
        }

        return;
    }
    
    /* Otherwise, take the damage */
    if (have_flag(samurai_slaying_ptr->flags, TR_BRAND_COLD)) {
        if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_COLD) {
            if (samurai_slaying_ptr->mult < 70)
                samurai_slaying_ptr->mult = 70;
            if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
                samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_COLD;
            }
        } else if (samurai_slaying_ptr->mult < 35)
            samurai_slaying_ptr->mult = 35;

        return;
    }

    if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_COLD) {
        if (samurai_slaying_ptr->mult < 50)
            samurai_slaying_ptr->mult = 50;
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_COLD;
        }
    } else if (samurai_slaying_ptr->mult < 25)
        samurai_slaying_ptr->mult = 25;
}

/*!
 * @brief 雷撃鷲爪斬
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 * @return なし
 */
static void hissatsu_lightning_eagle(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_ELEC)
        return;

    /* Notice immunity */
    if (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flagsr |= (samurai_slaying_ptr->r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
        }

        return;
    }

    /* Otherwise, take the damage */
    if (have_flag(samurai_slaying_ptr->flags, TR_BRAND_ELEC)) {
        if (samurai_slaying_ptr->mult < 70)
            samurai_slaying_ptr->mult = 70;
    } else {
        if (samurai_slaying_ptr->mult < 50)
            samurai_slaying_ptr->mult = 50;
    }
}

/*!
 * @brief 赤流渦 (ペインバッカー)
 * @param attacker_ptr プレーヤーへの参照ポインタ
 * @param samurai_slaying_ptr スレイ計算に必要なパラメータ群への参照ポインタ
 * @return なし
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
 * @return なし
 */
static void hissatsu_keiun_kininken(player_type *attacker_ptr, samurai_slaying_type *samurai_slaying_ptr)
{
    if (samurai_slaying_ptr->mode != HISSATSU_UNDEAD)
        return;

    if (samurai_slaying_ptr->r_ptr->flags3 & RF3_UNDEAD) {
        if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
            samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_UNDEAD;
        }
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
MULTIPLY mult_hissatsu(player_type *attacker_ptr, MULTIPLY mult, BIT_FLAGS *flags, monster_type *m_ptr, combat_options mode)
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
    if (mult > 150)
        mult = 150;

    return mult;
}
