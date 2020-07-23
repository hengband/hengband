#include "mspell/improper-mspell-remover.h"
#include "game-option/birth-options.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags4.h"
#include "monster/smart-learn-types.h"
#include "mspell/element-resistance-checker.h"
#include "mspell/smart-mspell-util.h"
#include "player/player-race.h"
#include "status/element-resistance.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"

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

static void check_nether_resistance(player_type *target_ptr, msr_type *msr_ptr)
{
    if ((msr_ptr->smart & SM_RES_NETH) == 0)
        return;

    if (is_specific_player_race(target_ptr, RACE_SPECTRE)) {
        msr_ptr->f4 &= ~(RF4_BR_NETH);
        msr_ptr->f5 &= ~(RF5_BA_NETH);
        msr_ptr->f5 &= ~(RF5_BO_NETH);
        return;
    }

    if (int_outof(msr_ptr->r_ptr, 20))
        msr_ptr->f4 &= ~(RF4_BR_NETH);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f5 &= ~(RF5_BA_NETH);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f5 &= ~(RF5_BO_NETH);
}

static void check_lite_resistance(msr_type *msr_ptr)
{
    if ((msr_ptr->smart & SM_RES_LITE) == 0)
        return;

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f4 &= ~(RF4_BR_LITE);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f5 &= ~(RF5_BA_LITE);
}

static void check_dark_resistance(player_type *target_ptr, msr_type *msr_ptr)
{
    if ((msr_ptr->smart & SM_RES_DARK) == 0)
        return;

    if (is_specific_player_race(target_ptr, RACE_VAMPIRE)) {
        msr_ptr->f4 &= ~(RF4_BR_DARK);
        msr_ptr->f5 &= ~(RF5_BA_DARK);
        return;
    }

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f4 &= ~(RF4_BR_DARK);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f5 &= ~(RF5_BA_DARK);
}

static void check_conf_resistance(msr_type *msr_ptr)
{
    if ((msr_ptr->smart & SM_RES_CONF) == 0)
        return;

    msr_ptr->f5 &= ~(RF5_CONF);
    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f4 &= ~(RF4_BR_CONF);
}

static void check_chaos_resistance(msr_type *msr_ptr)
{
    if ((msr_ptr->smart & SM_RES_CHAOS) == 0)
        return;

    if (int_outof(msr_ptr->r_ptr, 20))
        msr_ptr->f4 &= ~(RF4_BR_CHAO);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f4 &= ~(RF4_BA_CHAO);
}

static void check_nexus_resistance(msr_type *msr_ptr)
{
    if ((msr_ptr->smart & SM_RES_NEXUS) == 0)
        return;

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f4 &= ~(RF4_BR_NEXU);

    msr_ptr->f6 &= ~(RF6_TELE_LEVEL);
}

static void check_reflection(msr_type *msr_ptr)
{
    if ((msr_ptr->smart & SM_IMM_REFLECT) == 0)
        return;

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

void check_high_resistances(player_type *target_ptr, msr_type *msr_ptr)
{
    check_nether_resistance(target_ptr, msr_ptr);
    check_lite_resistance(msr_ptr);
    check_dark_resistance(target_ptr, msr_ptr);
    if (msr_ptr->smart & SM_RES_FEAR)
        msr_ptr->f5 &= ~(RF5_SCARE);

    check_conf_resistance(msr_ptr);
    check_chaos_resistance(msr_ptr);
    if (((msr_ptr->smart & SM_RES_DISEN) != 0) && int_outof(msr_ptr->r_ptr, 40))
        msr_ptr->f4 &= ~(RF4_BR_DISE);

    if (msr_ptr->smart & SM_RES_BLIND)
        msr_ptr->f5 &= ~(RF5_BLIND);

    check_nexus_resistance(msr_ptr);
    if (((msr_ptr->smart & SM_RES_SOUND) != 0) && int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->f4 &= ~(RF4_BR_SOUN);

    if (((msr_ptr->smart & SM_RES_SHARD) != 0) && int_outof(msr_ptr->r_ptr, 40))
        msr_ptr->f4 &= ~(RF4_BR_SHAR);

    check_reflection(msr_ptr);
    if (msr_ptr->smart & SM_IMM_FREE) {
        msr_ptr->f5 &= ~(RF5_HOLD);
        msr_ptr->f5 &= ~(RF5_SLOW);
    }

    if (msr_ptr->smart & SM_IMM_MANA)
        msr_ptr->f5 &= ~(RF5_DRAIN_MANA);
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

    check_element_resistance(msr_ptr);
    check_high_resistances(target_ptr, msr_ptr);
    *f4p = msr_ptr->f4;
    *f5p = msr_ptr->f5;
    *f6p = msr_ptr->f6;
}
