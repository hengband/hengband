#include "hpmp/hp-mp-regenerator.h"
#include "cmd-item/cmd-magiceat.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster/monster-status.h"
#include "player-base/player-class.h"
#include "player-info/magic-eater-data-type.h"
#include "player-info/samurai-data-type.h"
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
void regenhp(PlayerType *player_ptr, int percent)
{
    if (PlayerClass(player_ptr).samurai_stance_is(SamuraiStanceType::KOUKIJIN))
        return;
    if (player_ptr->action == ACTION_HAYAGAKE)
        return;

    HIT_POINT old_chp = player_ptr->chp;

    /*
     * Extract the new hitpoints
     *
     * 'percent' is the Regen factor in unit (1/2^16)
     */
    HIT_POINT new_chp = 0;
    uint32_t new_chp_frac = (player_ptr->mhp * percent + PY_REGEN_HPBASE);
    s64b_lshift(&new_chp, &new_chp_frac, 16);
    s64b_add(&(player_ptr->chp), &(player_ptr->chp_frac), new_chp, new_chp_frac);
    if (0 < s64b_cmp(player_ptr->chp, player_ptr->chp_frac, player_ptr->mhp, 0)) {
        player_ptr->chp = player_ptr->mhp;
        player_ptr->chp_frac = 0;
    }

    if (old_chp != player_ptr->chp) {
        player_ptr->redraw |= (PR_HP);
        player_ptr->window_flags |= (PW_PLAYER);
        wild_regen = 20;
    }
}

/*!
 * @brief プレイヤーのMP自然回復処理(regen_magic()のサブセット) / Regenerate mana points
 * @param upkeep_factor ペット維持によるMPコスト量
 * @param regen_amount 回復量
 */
void regenmana(PlayerType *player_ptr, MANA_POINT upkeep_factor, MANA_POINT regen_amount)
{
    MANA_POINT old_csp = player_ptr->csp;
    int32_t regen_rate = regen_amount * 100 - upkeep_factor * PY_REGEN_NORMAL;

    /*
     * Excess mana will decay 32 times faster than normal
     * regeneration rate.
     */
    if (player_ptr->csp > player_ptr->msp) {
        int32_t decay = 0;
        uint32_t decay_frac = (player_ptr->msp * 32 * PY_REGEN_NORMAL + PY_REGEN_MNBASE);
        s64b_lshift(&decay, &decay_frac, 16);
        s64b_sub(&(player_ptr->csp), &(player_ptr->csp_frac), decay, decay_frac);
        if (player_ptr->csp < player_ptr->msp) {
            player_ptr->csp = player_ptr->msp;
            player_ptr->csp_frac = 0;
        }
    }

    /* Regenerating mana (unless the player has excess mana) */
    else if (regen_rate > 0) {
        MANA_POINT new_mana = 0;
        uint32_t new_mana_frac = (player_ptr->msp * regen_rate / 100 + PY_REGEN_MNBASE);
        s64b_lshift(&new_mana, &new_mana_frac, 16);
        s64b_add(&(player_ptr->csp), &(player_ptr->csp_frac), new_mana, new_mana_frac);
        if (player_ptr->csp >= player_ptr->msp) {
            player_ptr->csp = player_ptr->msp;
            player_ptr->csp_frac = 0;
        }
    }

    /* Reduce mana (even when the player has excess mana) */
    if (regen_rate < 0) {
        int32_t reduce_mana = 0;
        uint32_t reduce_mana_frac = (player_ptr->msp * (-1) * regen_rate / 100 + PY_REGEN_MNBASE);
        s64b_lshift(&reduce_mana, &reduce_mana_frac, 16);
        s64b_sub(&(player_ptr->csp), &(player_ptr->csp_frac), reduce_mana, reduce_mana_frac);
        if (player_ptr->csp < 0) {
            player_ptr->csp = 0;
            player_ptr->csp_frac = 0;
        }
    }

    if (old_csp != player_ptr->csp) {
        player_ptr->redraw |= (PR_MANA);
        player_ptr->window_flags |= (PW_PLAYER);
        player_ptr->window_flags |= (PW_SPELL);
        wild_regen = 20;
    }
}

/*!
 * @brief 取り込んだ魔道具の自然回復処理 / Regenerate magic regen_amount: PY_REGEN_NORMAL * 2 (if resting) * 2 (if having regenarate)
 * @param regen_amount 回復量
 */
void regenmagic(PlayerType *player_ptr, int regen_amount)
{
    auto magic_eater_data = PlayerClass(player_ptr).get_specific_data<magic_eater_data_type>();
    if (!magic_eater_data) {
        return;
    }

    const int dev = 30;
    const int mult = (dev + adj_mag_mana[player_ptr->stat_index[A_INT]]); /* x1 to x2 speed bonus for recharging */

    for (auto tval : { ItemKindType::STAFF, ItemKindType::WAND }) {
        for (auto &item : magic_eater_data->get_item_group(tval)) {
            const int maximum_charge = item.count * EATER_CHARGE;
            if (item.count == 0 || item.charge == maximum_charge) {
                continue;
            }

            /* Increase remaining charge number like float value */
            auto new_mana = (regen_amount * mult * (item.count + 13)) / (dev * 8);
            item.charge += new_mana;

            /* Check maximum charge */
            item.charge = std::min(item.charge, maximum_charge);
        }
    }

    for (auto &item : magic_eater_data->get_item_group(ItemKindType::ROD)) {
        if (item.count == 0 || item.charge == 0) {
            continue;
        }

        /* Decrease remaining period for charging */
        auto new_mana = (regen_amount * mult * (item.count + 10) * EATER_ROD_CHARGE) / (dev * 16 * PY_REGEN_NORMAL);
        item.charge -= new_mana;

        /* Check minimum remaining period for charging */
        item.charge = std::max(item.charge, 0);
    }

    wild_regen = 20;
}

/*!
 * @brief 100ゲームターン毎のモンスターのHP自然回復処理 / Regenerate the monsters (once per 100 game turns)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @note Should probably be done during monster turns.
 */
void regenerate_monsters(PlayerType *player_ptr)
{
    for (int i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        auto *r_ptr = &r_info[m_ptr->r_idx];

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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @note Should probably be done during monster turns.
 */
void regenerate_captured_monsters(PlayerType *player_ptr)
{
    bool heal = false;
    for (int i = 0; i < INVEN_TOTAL; i++) {
        monster_race *r_ptr;
        auto *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;
        if (o_ptr->tval != ItemKindType::CAPTURE)
            continue;
        if (!o_ptr->pval)
            continue;

        heal = true;
        r_ptr = &r_info[o_ptr->pval];
        if (o_ptr->captured_monster_current_hp < o_ptr->captured_monster_max_hp) {
            short frac = o_ptr->captured_monster_max_hp / 100;
            if (!frac)
                if (one_in_(2))
                    frac = 1;

            if (r_ptr->flags2 & RF2_REGENERATE)
                frac *= 2;

            o_ptr->captured_monster_current_hp += frac;
            if (o_ptr->captured_monster_current_hp > o_ptr->captured_monster_max_hp)
                o_ptr->captured_monster_current_hp = o_ptr->captured_monster_max_hp;
        }
    }

    if (heal) {
        player_ptr->update |= (PU_COMBINE);
        // FIXME 広域マップ移動で1歩毎に何度も再描画されて重くなる。現在はボール中モンスターのHP回復でボールの表示は変わらないためコメントアウトする。
        //player_ptr->window_flags |= (PW_INVEN);
        //player_ptr->window_flags |= (PW_EQUIP);
        wild_regen = 20;
    }
}
