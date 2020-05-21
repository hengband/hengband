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
    } else {
        if (samurai_slaying_ptr->r_ptr->flags3 & RF3_HURT_FIRE) {
            if (samurai_slaying_ptr->mult < 50)
                samurai_slaying_ptr->mult = 50;
            if (is_original_ap_and_seen(attacker_ptr, samurai_slaying_ptr->m_ptr)) {
                samurai_slaying_ptr->r_ptr->r_flags3 |= RF3_HURT_FIRE;
            }
        } else if (samurai_slaying_ptr->mult < 25)
            samurai_slaying_ptr->mult = 25;
    }
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

    /* Serpent's Tongue (Poison) */
    if (mode == HISSATSU_POISON) {
        /* Notice immunity */
        if (r_ptr->flagsr & RFR_EFF_IM_POIS_MASK) {
            if (is_original_ap_and_seen(attacker_ptr, m_ptr)) {
                r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_POIS_MASK);
            }
        }

        /* Otherwise, take the damage */
        else if (have_flag(flags, TR_BRAND_POIS)) {
            if (mult < 35)
                mult = 35;
        } else {
            if (mult < 25)
                mult = 25;
        }
    }

    /* Zammaken (Nonliving Evil) */
    if (mode == HISSATSU_ZANMA) {
        if (!monster_living(m_ptr->r_idx) && (r_ptr->flags3 & RF3_EVIL)) {
            if (mult < 15)
                mult = 25;
            else if (mult < 50)
                mult = MIN(50, mult + 20);
        }
    }

    /* Rock Smash (Hurt Rock) */
    if (mode == HISSATSU_HAGAN) {
        if (r_ptr->flags3 & RF3_HURT_ROCK) {
            if (is_original_ap_and_seen(attacker_ptr, m_ptr)) {
                r_ptr->r_flags3 |= RF3_HURT_ROCK;
            }
            if (mult == 10)
                mult = 40;
            else if (mult < 60)
                mult = 60;
        }
    }

    /* Midare-Setsugekka (Cold) */
    if (mode == HISSATSU_COLD) {
        /* Notice immunity */
        if (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK) {
            if (is_original_ap_and_seen(attacker_ptr, m_ptr)) {
                r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_COLD_MASK);
            }
        }
        /* Otherwise, take the damage */
        else if (have_flag(flags, TR_BRAND_COLD)) {
            if (r_ptr->flags3 & RF3_HURT_COLD) {
                if (mult < 70)
                    mult = 70;
                if (is_original_ap_and_seen(attacker_ptr, m_ptr)) {
                    r_ptr->r_flags3 |= RF3_HURT_COLD;
                }
            } else if (mult < 35)
                mult = 35;
        } else {
            if (r_ptr->flags3 & RF3_HURT_COLD) {
                if (mult < 50)
                    mult = 50;
                if (is_original_ap_and_seen(attacker_ptr, m_ptr)) {
                    r_ptr->r_flags3 |= RF3_HURT_COLD;
                }
            } else if (mult < 25)
                mult = 25;
        }
    }

    /* Lightning Eagle (Elec) */
    if (mode == HISSATSU_ELEC) {
        /* Notice immunity */
        if (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK) {
            if (is_original_ap_and_seen(attacker_ptr, m_ptr)) {
                r_ptr->r_flagsr |= (r_ptr->flagsr & RFR_EFF_IM_ELEC_MASK);
            }
        }

        /* Otherwise, take the damage */
        else if (have_flag(flags, TR_BRAND_ELEC)) {
            if (mult < 70)
                mult = 70;
        } else {
            if (mult < 50)
                mult = 50;
        }
    }

    /* Bloody Maelstrom */
    if ((mode == HISSATSU_SEKIRYUKA) && attacker_ptr->cut && monster_living(m_ptr->r_idx)) {
        MULTIPLY tmp = MIN(100, MAX(10, attacker_ptr->cut / 10));
        if (mult < tmp)
            mult = tmp;
    }

    /* Keiun-Kininken */
    if (mode == HISSATSU_UNDEAD) {
        if (r_ptr->flags3 & RF3_UNDEAD) {
            if (is_original_ap_and_seen(attacker_ptr, m_ptr)) {
                r_ptr->r_flags3 |= RF3_UNDEAD;
            }
            if (mult == 10)
                mult = 70;
            else if (mult < 140)
                mult = MIN(140, mult + 60);
        }
        if (mult == 10)
            mult = 40;
        else if (mult < 60)
            mult = MIN(60, mult + 30);
    }

    if (mult > 150)
        mult = 150;
    return mult;
}
