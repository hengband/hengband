#include "mspell/improper-mspell-remover.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags4.h"
#include "monster/smart-learn-types.h"
#include "player/player-race.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"

/*!
 * @brief モンスターがプレイヤーの弱点をついた選択を取るかどうかの判定 /
 * Internal probability routine
 * @param r_ptr モンスター種族の構造体参照ポインタ
 * @param prob 基本確率(%)
 * @return 適した選択を取るならばTRUEを返す。
 */
static bool int_outof(monster_race *r_ptr, PERCENTAGE prob)
{
    if (!(r_ptr->flags2 & RF2_SMART))
        prob = prob / 2;

    return (randint0(100) < prob);
}

/*!
 * @brief モンスターの魔法一覧から戦術的に適さない魔法を除外する /
 * Remove the "bad" spells from a spell list
 * @param m_idx モンスターの構造体参照ポインタ
 * @param f4p モンスター魔法のフラグリスト1
 * @param f5p モンスター魔法のフラグリスト2
 * @param f6p モンスター魔法のフラグリスト3
 * @return なし
 */
void remove_bad_spells(MONSTER_IDX m_idx, player_type *target_ptr, u32b *f4p, u32b *f5p, u32b *f6p)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];
    u32b f4 = (*f4p);
    u32b f5 = (*f5p);
    u32b f6 = (*f6p);
    u32b smart = 0L;
    if (r_ptr->flags2 & RF2_STUPID)
        return;

    if (!smart_cheat && !smart_learn)
        return;

    if (smart_learn) {
        if (m_ptr->smart && (randint0(100) < 1))
            m_ptr->smart &= (SM_FRIENDLY | SM_PET | SM_CLONED);

        smart = m_ptr->smart;
    }

    if (smart_cheat) {
        if (target_ptr->resist_acid)
            smart |= (SM_RES_ACID);
        if (is_oppose_acid(target_ptr))
            smart |= (SM_OPP_ACID);
        if (target_ptr->immune_acid)
            smart |= (SM_IMM_ACID);
        if (target_ptr->resist_elec)
            smart |= (SM_RES_ELEC);
        if (is_oppose_elec(target_ptr))
            smart |= (SM_OPP_ELEC);
        if (target_ptr->immune_elec)
            smart |= (SM_IMM_ELEC);
        if (target_ptr->resist_fire)
            smart |= (SM_RES_FIRE);
        if (is_oppose_fire(target_ptr))
            smart |= (SM_OPP_FIRE);
        if (target_ptr->immune_fire)
            smart |= (SM_IMM_FIRE);
        if (target_ptr->resist_cold)
            smart |= (SM_RES_COLD);
        if (is_oppose_cold(target_ptr))
            smart |= (SM_OPP_COLD);
        if (target_ptr->immune_cold)
            smart |= (SM_IMM_COLD);
        if (target_ptr->resist_pois)
            smart |= (SM_RES_POIS);
        if (is_oppose_pois(target_ptr))
            smart |= (SM_OPP_POIS);

        if (target_ptr->resist_neth)
            smart |= (SM_RES_NETH);
        if (target_ptr->resist_lite)
            smart |= (SM_RES_LITE);
        if (target_ptr->resist_dark)
            smart |= (SM_RES_DARK);
        if (target_ptr->resist_fear)
            smart |= (SM_RES_FEAR);
        if (target_ptr->resist_conf)
            smart |= (SM_RES_CONF);
        if (target_ptr->resist_chaos)
            smart |= (SM_RES_CHAOS);
        if (target_ptr->resist_disen)
            smart |= (SM_RES_DISEN);
        if (target_ptr->resist_blind)
            smart |= (SM_RES_BLIND);
        if (target_ptr->resist_nexus)
            smart |= (SM_RES_NEXUS);
        if (target_ptr->resist_sound)
            smart |= (SM_RES_SOUND);
        if (target_ptr->resist_shard)
            smart |= (SM_RES_SHARD);
        if (target_ptr->reflect)
            smart |= (SM_IMM_REFLECT);

        if (target_ptr->free_act)
            smart |= (SM_IMM_FREE);
        if (!target_ptr->msp)
            smart |= (SM_IMM_MANA);
    }

    if (!smart)
        return;

    if (smart & SM_IMM_ACID) {
        f4 &= ~(RF4_BR_ACID);
        f5 &= ~(RF5_BA_ACID);
        f5 &= ~(RF5_BO_ACID);
    } else if ((smart & (SM_OPP_ACID)) && (smart & (SM_RES_ACID))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_ACID);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_ACID);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_ACID);
    } else if ((smart & (SM_OPP_ACID)) || (smart & (SM_RES_ACID))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_ACID);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_ACID);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BO_ACID);
    }

    if (smart & (SM_IMM_ELEC)) {
        f4 &= ~(RF4_BR_ELEC);
        f5 &= ~(RF5_BA_ELEC);
        f5 &= ~(RF5_BO_ELEC);
    } else if ((smart & (SM_OPP_ELEC)) && (smart & (SM_RES_ELEC))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_ELEC);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_ELEC);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_ELEC);
    } else if ((smart & (SM_OPP_ELEC)) || (smart & (SM_RES_ELEC))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_ELEC);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_ELEC);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BO_ELEC);
    }

    if (smart & (SM_IMM_FIRE)) {
        f4 &= ~(RF4_BR_FIRE);
        f5 &= ~(RF5_BA_FIRE);
        f5 &= ~(RF5_BO_FIRE);
    } else if ((smart & (SM_OPP_FIRE)) && (smart & (SM_RES_FIRE))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_FIRE);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_FIRE);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_FIRE);
    } else if ((smart & (SM_OPP_FIRE)) || (smart & (SM_RES_FIRE))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_FIRE);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_FIRE);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BO_FIRE);
    }

    if (smart & (SM_IMM_COLD)) {
        f4 &= ~(RF4_BR_COLD);
        f5 &= ~(RF5_BA_COLD);
        f5 &= ~(RF5_BO_COLD);
        f5 &= ~(RF5_BO_ICEE);
    } else if ((smart & (SM_OPP_COLD)) && (smart & (SM_RES_COLD))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_COLD);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_COLD);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_COLD);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BO_ICEE);
    } else if ((smart & (SM_OPP_COLD)) || (smart & (SM_RES_COLD))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_COLD);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_COLD);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BO_COLD);
        if (int_outof(r_ptr, 20))
            f5 &= ~(RF5_BO_ICEE);
    }

    if ((smart & (SM_OPP_POIS)) && (smart & (SM_RES_POIS))) {
        if (int_outof(r_ptr, 80))
            f4 &= ~(RF4_BR_POIS);
        if (int_outof(r_ptr, 80))
            f5 &= ~(RF5_BA_POIS);
        if (int_outof(r_ptr, 60))
            f4 &= ~(RF4_BA_NUKE);
        if (int_outof(r_ptr, 60))
            f4 &= ~(RF4_BR_NUKE);
    } else if ((smart & (SM_OPP_POIS)) || (smart & (SM_RES_POIS))) {
        if (int_outof(r_ptr, 30))
            f4 &= ~(RF4_BR_POIS);
        if (int_outof(r_ptr, 30))
            f5 &= ~(RF5_BA_POIS);
    }

    if (smart & (SM_RES_NETH)) {
        if (is_specific_player_race(target_ptr, RACE_SPECTRE)) {
            f4 &= ~(RF4_BR_NETH);
            f5 &= ~(RF5_BA_NETH);
            f5 &= ~(RF5_BO_NETH);
        } else {
            if (int_outof(r_ptr, 20))
                f4 &= ~(RF4_BR_NETH);
            if (int_outof(r_ptr, 50))
                f5 &= ~(RF5_BA_NETH);
            if (int_outof(r_ptr, 50))
                f5 &= ~(RF5_BO_NETH);
        }
    }

    if (smart & (SM_RES_LITE)) {
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BR_LITE);
        if (int_outof(r_ptr, 50))
            f5 &= ~(RF5_BA_LITE);
    }

    if (smart & (SM_RES_DARK)) {
        if (is_specific_player_race(target_ptr, RACE_VAMPIRE)) {
            f4 &= ~(RF4_BR_DARK);
            f5 &= ~(RF5_BA_DARK);
        } else {
            if (int_outof(r_ptr, 50))
                f4 &= ~(RF4_BR_DARK);
            if (int_outof(r_ptr, 50))
                f5 &= ~(RF5_BA_DARK);
        }
    }

    if (smart & (SM_RES_FEAR)) {
        f5 &= ~(RF5_SCARE);
    }

    if (smart & (SM_RES_CONF)) {
        f5 &= ~(RF5_CONF);
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BR_CONF);
    }

    if (smart & (SM_RES_CHAOS)) {
        if (int_outof(r_ptr, 20))
            f4 &= ~(RF4_BR_CHAO);
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BA_CHAO);
    }

    if (smart & (SM_RES_DISEN)) {
        if (int_outof(r_ptr, 40))
            f4 &= ~(RF4_BR_DISE);
    }

    if (smart & (SM_RES_BLIND)) {
        f5 &= ~(RF5_BLIND);
    }

    if (smart & (SM_RES_NEXUS)) {
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BR_NEXU);
        f6 &= ~(RF6_TELE_LEVEL);
    }

    if (smart & (SM_RES_SOUND)) {
        if (int_outof(r_ptr, 50))
            f4 &= ~(RF4_BR_SOUN);
    }

    if (smart & (SM_RES_SHARD)) {
        if (int_outof(r_ptr, 40))
            f4 &= ~(RF4_BR_SHAR);
    }

    if (smart & (SM_IMM_REFLECT)) {
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_COLD);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_FIRE);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_ACID);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_ELEC);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_NETH);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_WATE);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_MANA);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_PLAS);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_BO_ICEE);
        if (int_outof(r_ptr, 150))
            f5 &= ~(RF5_MISSILE);
    }

    if (smart & (SM_IMM_FREE)) {
        f5 &= ~(RF5_HOLD);
        f5 &= ~(RF5_SLOW);
    }

    if (smart & (SM_IMM_MANA)) {
        f5 &= ~(RF5_DRAIN_MANA);
    }

    (*f4p) = f4;
    (*f5p) = f5;
    (*f6p) = f6;
}
