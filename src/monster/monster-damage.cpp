/*
 * @brief モンスターがダメージを受けた時の処理と経験値の加算処理
 * @date 2021/08/04
 * @author Hourier
 */

#include "monster/monster-damage.h"
#include "core/speed-table.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags8.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief モンスターに与えたダメージを元に経験値を加算する /
 * Calculate experience point to be get
 * @param dam 与えたダメージ量
 * @param m_ptr ダメージを与えたモンスターの構造体参照ポインタ
 * @details
 * <pre>
 * Even the 64 bit operation is not big enough to avoid overflaw
 * unless we carefully choose orders of ENERGY_MULTIPLICATION and ENERGY_DIVISION.
 * Get the coefficient first, and multiply (potentially huge) base
 * experience point of a monster later.
 * </pre>
 */
void get_exp_from_mon(player_type *target_ptr, HIT_POINT dam, monster_type *m_ptr)
{
    auto *r_ptr = &r_info[m_ptr->r_idx];
    if (!monster_is_valid(m_ptr) || is_pet(m_ptr) || target_ptr->phase_out) {
        return;
    }

    /*
     * - Ratio of monster's level to player's level effects
     * - Varying speed effects
     * - Get a fraction in proportion of damage point
     */
    auto new_exp = r_ptr->level * SPEED_TO_ENERGY(m_ptr->mspeed) * dam;
    auto new_exp_frac = 0U;
    auto div_h = 0;
    auto div_l = (uint)((target_ptr->max_plv + 2) * SPEED_TO_ENERGY(r_ptr->speed));

    /* Use (average maxhp * 2) as a denominator */
    auto compensation = any_bits(r_ptr->flags1, RF1_FORCE_MAXHP) ? r_ptr->hside * 2 : r_ptr->hside + 1;
    s64b_mul(&div_h, &div_l, 0, r_ptr->hdice * (ironman_nightmare ? 2 : 1) * compensation);

    /* Special penalty in the wilderness */
    if (!target_ptr->current_floor_ptr->dun_level && (none_bits(r_ptr->flags8, RF8_WILD_ONLY) || none_bits(r_ptr->flags1, RF1_UNIQUE))) {
        s64b_mul(&div_h, &div_l, 0, 5);
    }

    /* Do ENERGY_DIVISION first to prevent overflaw */
    s64b_div(&new_exp, &new_exp_frac, div_h, div_l);

    /* Special penalty for mutiply-monster */
    if (any_bits(r_ptr->flags2, RF2_MULTIPLY) || (m_ptr->r_idx == MON_DAWN)) {
        int monnum_penarty = r_ptr->r_akills / 400;
        if (monnum_penarty > 8) {
            monnum_penarty = 8;
        }

        while (monnum_penarty--) {
            /* Divide by 4 */
            s64b_rshift(&new_exp, &new_exp_frac, 2);
        }
    }

    /* Special penalty for rest_and_shoot exp scum */
    if ((m_ptr->dealt_damage > m_ptr->max_maxhp) && (m_ptr->hp >= 0)) {
        int over_damage = m_ptr->dealt_damage / m_ptr->max_maxhp;
        if (over_damage > 32) {
            over_damage = 32;
        }

        while (over_damage--) {
            /* 9/10 for once */
            s64b_mul(&new_exp, &new_exp_frac, 0, 9);
            s64b_div(&new_exp, &new_exp_frac, 0, 10);
        }
    }

    s64b_mul(&new_exp, &new_exp_frac, 0, r_ptr->mexp);
    gain_exp_64(target_ptr, new_exp, new_exp_frac);
}
