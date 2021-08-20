#include "hpmp/hp-mp-regenerator.h"
#include "cmd-item/cmd-magiceat.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-status.h"
#include "player/attack-defense-types.h"
#include "player/player-status-table.h"
#include "player/special-defense-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"

/*!<広域マップ移動時の自然回復処理カウンタ（広域マップ1マス毎に20回処理を基本とする）*/
int wild_regen = 20;

/*!
 * @brief プレイヤーのHP自然回復処理 / Regenerate hit points -RAK-
 * @param percent 回復比率
 */
void regenhp(player_type *creature_ptr, int percent)
{
    if (creature_ptr->special_defense & KATA_KOUKIJIN)
        return;
    if (creature_ptr->action == ACTION_HAYAGAKE)
        return;

    HIT_POINT old_chp = creature_ptr->chp;

    /*
     * Extract the new hitpoints
     *
     * 'percent' is the Regen factor in unit (1/2^16)
     */
    HIT_POINT new_chp = 0;
    uint32_t new_chp_frac = (creature_ptr->mhp * percent + PY_REGEN_HPBASE);
    s64b_lshift(&new_chp, &new_chp_frac, 16);
    s64b_add(&(creature_ptr->chp), &(creature_ptr->chp_frac), new_chp, new_chp_frac);
    if (0 < s64b_cmp(creature_ptr->chp, creature_ptr->chp_frac, creature_ptr->mhp, 0)) {
        creature_ptr->chp = creature_ptr->mhp;
        creature_ptr->chp_frac = 0;
    }

    if (old_chp != creature_ptr->chp) {
        creature_ptr->redraw |= (PR_HP);
        creature_ptr->window_flags |= (PW_PLAYER);
        wild_regen = 20;
    }
}

/*!
 * @brief プレイヤーのMP自然回復処理(regen_magic()のサブセット) / Regenerate mana points
 * @param upkeep_factor ペット維持によるMPコスト量
 * @param regen_amount 回復量
 */
void regenmana(player_type *creature_ptr, MANA_POINT upkeep_factor, MANA_POINT regen_amount)
{
    MANA_POINT old_csp = creature_ptr->csp;
    int32_t regen_rate = regen_amount * 100 - upkeep_factor * PY_REGEN_NORMAL;

    /*
     * Excess mana will decay 32 times faster than normal
     * regeneration rate.
     */
    if (creature_ptr->csp > creature_ptr->msp) {
        int32_t decay = 0;
        uint32_t decay_frac = (creature_ptr->msp * 32 * PY_REGEN_NORMAL + PY_REGEN_MNBASE);
        s64b_lshift(&decay, &decay_frac, 16);
        s64b_sub(&(creature_ptr->csp), &(creature_ptr->csp_frac), decay, decay_frac);
        if (creature_ptr->csp < creature_ptr->msp) {
            creature_ptr->csp = creature_ptr->msp;
            creature_ptr->csp_frac = 0;
        }
    }

    /* Regenerating mana (unless the player has excess mana) */
    else if (regen_rate > 0) {
        MANA_POINT new_mana = 0;
        uint32_t new_mana_frac = (creature_ptr->msp * regen_rate / 100 + PY_REGEN_MNBASE);
        s64b_lshift(&new_mana, &new_mana_frac, 16);
        s64b_add(&(creature_ptr->csp), &(creature_ptr->csp_frac), new_mana, new_mana_frac);
        if (creature_ptr->csp >= creature_ptr->msp) {
            creature_ptr->csp = creature_ptr->msp;
            creature_ptr->csp_frac = 0;
        }
    }

    /* Reduce mana (even when the player has excess mana) */
    if (regen_rate < 0) {
        int32_t reduce_mana = 0;
        uint32_t reduce_mana_frac = (creature_ptr->msp * (-1) * regen_rate / 100 + PY_REGEN_MNBASE);
        s64b_lshift(&reduce_mana, &reduce_mana_frac, 16);
        s64b_sub(&(creature_ptr->csp), &(creature_ptr->csp_frac), reduce_mana, reduce_mana_frac);
        if (creature_ptr->csp < 0) {
            creature_ptr->csp = 0;
            creature_ptr->csp_frac = 0;
        }
    }

    if (old_csp != creature_ptr->csp) {
        creature_ptr->redraw |= (PR_MANA);
        creature_ptr->window_flags |= (PW_PLAYER);
        creature_ptr->window_flags |= (PW_SPELL);
        wild_regen = 20;
    }
}

/*!
 * @brief プレイヤーのMP自然回復処理 / Regenerate magic regen_amount: PY_REGEN_NORMAL * 2 (if resting) * 2 (if having regenarate)
 * @param regen_amount 回復量
 */
void regenmagic(player_type *creature_ptr, int regen_amount)
{
    MANA_POINT new_mana;
    int dev = 30;
    int mult = (dev + adj_mag_mana[creature_ptr->stat_index[A_INT]]); /* x1 to x2 speed bonus for recharging */

    for (int i = 0; i < EATER_EXT * 2; i++) {
        if (!creature_ptr->magic_num2[i])
            continue;
        if (creature_ptr->magic_num1[i] == ((long)creature_ptr->magic_num2[i] << 16))
            continue;

        /* Increase remaining charge number like float value */
        new_mana = (regen_amount * mult * ((long)creature_ptr->magic_num2[i] + 13)) / (dev * 8);
        creature_ptr->magic_num1[i] += new_mana;

        /* Check maximum charge */
        if (creature_ptr->magic_num1[i] > (creature_ptr->magic_num2[i] << 16)) {
            creature_ptr->magic_num1[i] = ((long)creature_ptr->magic_num2[i] << 16);
        }

        wild_regen = 20;
    }

    for (int i = EATER_EXT * 2; i < EATER_EXT * 3; i++) {
        if (!creature_ptr->magic_num1[i])
            continue;
        if (!creature_ptr->magic_num2[i])
            continue;

        /* Decrease remaining period for charging */
        new_mana = (regen_amount * mult * ((long)creature_ptr->magic_num2[i] + 10) * EATER_ROD_CHARGE) / (dev * 16 * PY_REGEN_NORMAL);
        creature_ptr->magic_num1[i] -= new_mana;

        /* Check minimum remaining period for charging */
        if (creature_ptr->magic_num1[i] < 0)
            creature_ptr->magic_num1[i] = 0;
        wild_regen = 20;
    }
}

/*!
 * @brief 100ゲームターン毎のモンスターのHP自然回復処理 / Regenerate the monsters (once per 100 game turns)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @note Should probably be done during monster turns.
 */
void regenerate_monsters(player_type *player_ptr)
{
    for (int i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        monster_race *r_ptr = &r_info[m_ptr->r_idx];

        if (!monster_is_valid(m_ptr))
            continue;

        if (m_ptr->hp < m_ptr->maxhp) {
            int frac = m_ptr->maxhp / 100;
            if (!frac)
                if (one_in_(2))
                    frac = 1;

            if (r_ptr->flags2 & RF2_REGENERATE)
                frac *= 2;

            m_ptr->hp += frac;
            if (m_ptr->hp > m_ptr->maxhp)
                m_ptr->hp = m_ptr->maxhp;

            if (player_ptr->health_who == i)
                player_ptr->redraw |= (PR_HEALTH);
            if (player_ptr->riding == i)
                player_ptr->redraw |= (PR_UHEALTH);
        }
    }
}

/*!
 * @brief 30ゲームターン毎のボール中モンスターのHP自然回復処理 / Regenerate the captured monsters (once per 30 game turns)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @note Should probably be done during monster turns.
 */
void regenerate_captured_monsters(player_type *creature_ptr)
{
    bool heal = false;
    for (int i = 0; i < INVEN_TOTAL; i++) {
        monster_race *r_ptr;
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        if (o_ptr->tval != TV_CAPTURE)
            continue;
        if (!o_ptr->pval)
            continue;

        heal = true;
        r_ptr = &r_info[o_ptr->pval];
        if (o_ptr->xtra4 < o_ptr->xtra5) {
            int frac = o_ptr->xtra5 / 100;
            if (!frac)
                if (one_in_(2))
                    frac = 1;

            if (r_ptr->flags2 & RF2_REGENERATE)
                frac *= 2;

            o_ptr->xtra4 += (XTRA16)frac;
            if (o_ptr->xtra4 > o_ptr->xtra5)
                o_ptr->xtra4 = o_ptr->xtra5;
        }
    }

    if (heal) {
        creature_ptr->update |= (PU_COMBINE);
        // FIXME 広域マップ移動で1歩毎に何度も再描画されて重くなる。現在はボール中モンスターのHP回復でボールの表示は変わらないためコメントアウトする。
        //creature_ptr->window_flags |= (PW_INVEN);
        //creature_ptr->window_flags |= (PW_EQUIP);
        wild_regen = 20;
    }
}
