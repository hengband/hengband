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

// Monster Spell Remover.
typedef struct msr_type {
    monster_race *r_ptr;
    u32b f4;
    u32b f5;
    u32b f6;
    u32b smart;
} msr_type;

static msr_type *initialize_msr_type(player_type *target_ptr, msr_type *msr_ptr, MONSTER_IDX m_idx, const u32b f4p, const u32b f5p, const u32b f6p)
{
    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    msr_ptr->r_ptr = &r_info[m_ptr->r_idx];
    msr_ptr->f4 = f4p;
    msr_ptr->f5 = f5p;
    msr_ptr->f6 = f6p;
    msr_ptr->smart = 0L;
    return msr_ptr;
}

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

static void add_cheat_remove_flags_element(player_type *target_ptr, msr_type *msr_ptr)
{
    if (target_ptr->resist_acid)
        msr_ptr->smart |= SM_RES_ACID;

    if (is_oppose_acid(target_ptr))
        msr_ptr->smart |= SM_OPP_ACID;

    if (target_ptr->immune_acid)
        msr_ptr->smart |= SM_IMM_ACID;

    if (target_ptr->resist_elec)
        msr_ptr->smart |= SM_RES_ELEC;

    if (is_oppose_elec(target_ptr))
        msr_ptr->smart |= SM_OPP_ELEC;

    if (target_ptr->immune_elec)
        msr_ptr->smart |= SM_IMM_ELEC;

    if (target_ptr->resist_fire)
        msr_ptr->smart |= SM_RES_FIRE;

    if (is_oppose_fire(target_ptr))
        msr_ptr->smart |= SM_OPP_FIRE;

    if (target_ptr->immune_fire)
        msr_ptr->smart |= SM_IMM_FIRE;

    if (target_ptr->resist_cold)
        msr_ptr->smart |= SM_RES_COLD;

    if (is_oppose_cold(target_ptr))
        msr_ptr->smart |= SM_OPP_COLD;

    if (target_ptr->immune_cold)
        msr_ptr->smart |= SM_IMM_COLD;

    if (target_ptr->resist_pois)
        msr_ptr->smart |= SM_RES_POIS;

    if (is_oppose_pois(target_ptr))
        msr_ptr->smart |= SM_OPP_POIS;
}

static void add_cheat_remove_flags_others(player_type *target_ptr, msr_type *msr_ptr)
{
    if (target_ptr->resist_neth)
        msr_ptr->smart |= SM_RES_NETH;

    if (target_ptr->resist_lite)
        msr_ptr->smart |= SM_RES_LITE;

    if (target_ptr->resist_dark)
        msr_ptr->smart |= SM_RES_DARK;

    if (target_ptr->resist_fear)
        msr_ptr->smart |= SM_RES_FEAR;

    if (target_ptr->resist_conf)
        msr_ptr->smart |= SM_RES_CONF;

    if (target_ptr->resist_chaos)
        msr_ptr->smart |= SM_RES_CHAOS;

    if (target_ptr->resist_disen)
        msr_ptr->smart |= SM_RES_DISEN;

    if (target_ptr->resist_blind)
        msr_ptr->smart |= SM_RES_BLIND;

    if (target_ptr->resist_nexus)
        msr_ptr->smart |= SM_RES_NEXUS;

    if (target_ptr->resist_sound)
        msr_ptr->smart |= SM_RES_SOUND;

    if (target_ptr->resist_shard)
        msr_ptr->smart |= SM_RES_SHARD;

    if (target_ptr->reflect)
        msr_ptr->smart |= SM_IMM_REFLECT;

    if (target_ptr->free_act)
        msr_ptr->smart |= SM_IMM_FREE;

    if (!target_ptr->msp)
        msr_ptr->smart |= SM_IMM_MANA;
}

static void add_cheat_remove_flags(player_type *target_ptr, msr_type *msr_ptr)
{
    if (!smart_cheat)
        return;

    add_cheat_remove_flags_element(target_ptr, msr_ptr);
    add_cheat_remove_flags_others(target_ptr, msr_ptr);
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
    msr_type tmp_msr;
    msr_type *msr_ptr = initialize_msr_type(target_ptr, &tmp_msr, m_idx, *f4p, *f5p, *f6p);
    if (msr_ptr->r_ptr->flags2 & RF2_STUPID)
        return;

    if (!smart_cheat && !smart_learn)
        return;

    monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
    if (smart_learn) {
        if (m_ptr->smart && (randint0(100) < 1))
            m_ptr->smart &= SM_FRIENDLY | SM_PET | SM_CLONED;

        msr_ptr->smart = m_ptr->smart;
    }

    add_cheat_remove_flags(target_ptr, msr_ptr);
    if (!msr_ptr->smart)
        return;

    if (msr_ptr->smart & SM_IMM_ACID) {
        msr_ptr->f4 &= ~(RF4_BR_ACID);
        msr_ptr->f5 &= ~(RF5_BA_ACID);
        msr_ptr->f5 &= ~(RF5_BO_ACID);
    } else if ((msr_ptr->smart & SM_OPP_ACID) && (msr_ptr->smart & SM_RES_ACID)) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f4 &= ~(RF4_BR_ACID);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BA_ACID);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BO_ACID);
    } else if ((msr_ptr->smart & SM_OPP_ACID) || (msr_ptr->smart & SM_RES_ACID)) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f4 &= ~(RF4_BR_ACID);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BA_ACID);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BO_ACID);
    }

    if (msr_ptr->smart & SM_IMM_ELEC) {
        msr_ptr->f4 &= ~(RF4_BR_ELEC);
        msr_ptr->f5 &= ~(RF5_BA_ELEC);
        msr_ptr->f5 &= ~(RF5_BO_ELEC);
    } else if ((msr_ptr->smart & SM_OPP_ELEC) && (msr_ptr->smart & SM_RES_ELEC)) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f4 &= ~(RF4_BR_ELEC);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BA_ELEC);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BO_ELEC);
    } else if ((msr_ptr->smart & SM_OPP_ELEC) || (msr_ptr->smart & SM_RES_ELEC)) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f4 &= ~(RF4_BR_ELEC);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BA_ELEC);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BO_ELEC);
    }

    if (msr_ptr->smart & (SM_IMM_FIRE)) {
        msr_ptr->f4 &= ~(RF4_BR_FIRE);
        msr_ptr->f5 &= ~(RF5_BA_FIRE);
        msr_ptr->f5 &= ~(RF5_BO_FIRE);
    } else if ((msr_ptr->smart & SM_OPP_FIRE) && (msr_ptr->smart & SM_RES_FIRE)) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f4 &= ~(RF4_BR_FIRE);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BA_FIRE);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BO_FIRE);
    } else if ((msr_ptr->smart & SM_OPP_FIRE) || (msr_ptr->smart & SM_RES_FIRE)) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f4 &= ~(RF4_BR_FIRE);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BA_FIRE);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BO_FIRE);
    }

    if (msr_ptr->smart & (SM_IMM_COLD)) {
        msr_ptr->f4 &= ~(RF4_BR_COLD);
        msr_ptr->f5 &= ~(RF5_BA_COLD);
        msr_ptr->f5 &= ~(RF5_BO_COLD);
        msr_ptr->f5 &= ~(RF5_BO_ICEE);
    } else if ((msr_ptr->smart & SM_OPP_COLD) && (msr_ptr->smart & SM_RES_COLD)) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f4 &= ~(RF4_BR_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BA_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BO_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BO_ICEE);
    } else if ((msr_ptr->smart & SM_OPP_COLD) || (msr_ptr->smart & SM_RES_COLD)) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f4 &= ~(RF4_BR_COLD);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BA_COLD);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BO_COLD);

        if (int_outof(msr_ptr->r_ptr, 20))
            msr_ptr->f5 &= ~(RF5_BO_ICEE);
    }

    if ((msr_ptr->smart & SM_OPP_POIS) && (msr_ptr->smart & SM_RES_POIS)) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f4 &= ~(RF4_BR_POIS);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->f5 &= ~(RF5_BA_POIS);

        if (int_outof(msr_ptr->r_ptr, 60))
            msr_ptr->f4 &= ~(RF4_BA_NUKE);

        if (int_outof(msr_ptr->r_ptr, 60))
            msr_ptr->f4 &= ~(RF4_BR_NUKE);
    } else if ((msr_ptr->smart & SM_OPP_POIS) || (msr_ptr->smart & SM_RES_POIS)) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f4 &= ~(RF4_BR_POIS);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->f5 &= ~(RF5_BA_POIS);
    }

    if (msr_ptr->smart & SM_RES_NETH) {
        if (is_specific_player_race(target_ptr, RACE_SPECTRE)) {
            msr_ptr->f4 &= ~(RF4_BR_NETH);
            msr_ptr->f5 &= ~(RF5_BA_NETH);
            msr_ptr->f5 &= ~(RF5_BO_NETH);
        } else {
            if (int_outof(msr_ptr->r_ptr, 20))
                msr_ptr->f4 &= ~(RF4_BR_NETH);

            if (int_outof(msr_ptr->r_ptr, 50))
                msr_ptr->f5 &= ~(RF5_BA_NETH);

            if (int_outof(msr_ptr->r_ptr, 50))
                msr_ptr->f5 &= ~(RF5_BO_NETH);
        }
    }

    if (msr_ptr->smart & SM_RES_LITE) {
        if (int_outof(msr_ptr->r_ptr, 50))
            msr_ptr->f4 &= ~(RF4_BR_LITE);

        if (int_outof(msr_ptr->r_ptr, 50))
            msr_ptr->f5 &= ~(RF5_BA_LITE);
    }

    if (msr_ptr->smart & SM_RES_DARK) {
        if (is_specific_player_race(target_ptr, RACE_VAMPIRE)) {
            msr_ptr->f4 &= ~(RF4_BR_DARK);
            msr_ptr->f5 &= ~(RF5_BA_DARK);
        } else {
            if (int_outof(msr_ptr->r_ptr, 50))
                msr_ptr->f4 &= ~(RF4_BR_DARK);

            if (int_outof(msr_ptr->r_ptr, 50))
                msr_ptr->f5 &= ~(RF5_BA_DARK);
        }
    }

    if (msr_ptr->smart & SM_RES_FEAR) {
        msr_ptr->f5 &= ~(RF5_SCARE);
    }

    if (msr_ptr->smart & SM_RES_CONF) {
        msr_ptr->f5 &= ~(RF5_CONF);
        if (int_outof(msr_ptr->r_ptr, 50))
            msr_ptr->f4 &= ~(RF4_BR_CONF);
    }

    if (msr_ptr->smart & SM_RES_CHAOS) {
        if (int_outof(msr_ptr->r_ptr, 20))
            msr_ptr->f4 &= ~(RF4_BR_CHAO);

        if (int_outof(msr_ptr->r_ptr, 50))
            msr_ptr->f4 &= ~(RF4_BA_CHAO);
    }

    if (((msr_ptr->smart & SM_RES_DISEN) != 0) && int_outof(msr_ptr->r_ptr, 40))
        msr_ptr->f4 &= ~(RF4_BR_DISE);

    if (msr_ptr->smart & SM_RES_BLIND)
        msr_ptr->f5 &= ~(RF5_BLIND);

    if (msr_ptr->smart & SM_RES_NEXUS) {
        if (int_outof(msr_ptr->r_ptr, 50))
            msr_ptr->f4 &= ~(RF4_BR_NEXU);

        msr_ptr->f6 &= ~(RF6_TELE_LEVEL);
    }

    if (((msr_ptr->smart & SM_RES_SOUND) != 0) && int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f4 &= ~(RF4_BR_SOUN);

    if (((msr_ptr->smart & SM_RES_SHARD) != 0) && int_outof(msr_ptr->r_ptr, 40))
        msr_ptr->f4 &= ~(RF4_BR_SHAR);

    if (msr_ptr->smart & SM_IMM_REFLECT) {
        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_COLD);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_FIRE);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_ACID);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_ELEC);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_NETH);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_WATE);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_MANA);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_PLAS);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_BO_ICEE);

        if (int_outof(msr_ptr->r_ptr, 150))
            msr_ptr->f5 &= ~(RF5_MISSILE);
    }

    if (msr_ptr->smart & SM_IMM_FREE) {
        msr_ptr->f5 &= ~(RF5_HOLD);
        msr_ptr->f5 &= ~(RF5_SLOW);
    }

    if (msr_ptr->smart & SM_IMM_MANA)
        msr_ptr->f5 &= ~(RF5_DRAIN_MANA);

    *f4p = msr_ptr->f4;
    *f5p = msr_ptr->f5;
    *f6p = msr_ptr->f6;
}
