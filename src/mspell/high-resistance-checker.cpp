﻿#include "mspell/high-resistance-checker.h"
#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "mspell/smart-mspell-util.h"
#include "player-base/player-race.h"
#include "player/player-status-flags.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

void add_cheat_remove_flags_others(PlayerType *player_ptr, msr_type *msr_ptr)
{
    if (has_resist_neth(player_ptr))
        msr_ptr->smart.set(SM::RES_NETH);

    if (has_resist_lite(player_ptr))
        msr_ptr->smart.set(SM::RES_LITE);

    if (has_resist_dark(player_ptr))
        msr_ptr->smart.set(SM::RES_DARK);

    if (has_resist_fear(player_ptr))
        msr_ptr->smart.set(SM::RES_FEAR);

    if (has_resist_conf(player_ptr))
        msr_ptr->smart.set(SM::RES_CONF);

    if (has_resist_chaos(player_ptr))
        msr_ptr->smart.set(SM::RES_CHAOS);

    if (has_resist_disen(player_ptr))
        msr_ptr->smart.set(SM::RES_DISEN);

    if (has_resist_blind(player_ptr))
        msr_ptr->smart.set(SM::RES_BLIND);

    if (has_resist_nexus(player_ptr))
        msr_ptr->smart.set(SM::RES_NEXUS);

    if (has_resist_sound(player_ptr))
        msr_ptr->smart.set(SM::RES_SOUND);

    if (has_resist_shard(player_ptr))
        msr_ptr->smart.set(SM::RES_SHARD);

    if (has_reflect(player_ptr))
        msr_ptr->smart.set(SM::IMM_REFLECT);

    if (player_ptr->free_act)
        msr_ptr->smart.set(SM::IMM_FREE);

    if (!player_ptr->msp)
        msr_ptr->smart.set(SM::IMM_MANA);
}

static void check_nether_resistance(PlayerType *player_ptr, msr_type *msr_ptr)
{
    if (msr_ptr->smart.has_not(SM::RES_NETH))
        return;

    if (PlayerRace(player_ptr).equals(PlayerRaceType::SPECTRE)) {
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_NETH);
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_NETH);
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_NETH);
        return;
    }

    if (int_outof(msr_ptr->r_ptr, 20))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_NETH);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_NETH);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_NETH);
}

static void check_lite_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has_not(SM::RES_LITE))
        return;

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_LITE);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_LITE);
}

static void check_dark_resistance(PlayerType *player_ptr, msr_type *msr_ptr)
{
    if (msr_ptr->smart.has_not(SM::RES_DARK))
        return;

    if (PlayerRace(player_ptr).tr_flags().has(TR_IM_DARK)) {
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_DARK);
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_DARK);
        return;
    }

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_DARK);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_DARK);
}

static void check_conf_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has_not(SM::RES_CONF))
        return;

    msr_ptr->ability_flags.reset(RF_ABILITY::CONF);
    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_CONF);
}

static void check_chaos_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has_not(SM::RES_CHAOS))
        return;

    if (int_outof(msr_ptr->r_ptr, 20))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_CHAO);

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_CHAO);
}

static void check_nexus_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has_not(SM::RES_NEXUS))
        return;

    if (int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_NEXU);

    msr_ptr->ability_flags.reset(RF_ABILITY::TELE_LEVEL);
}

static void check_reflection(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has_not(SM::IMM_REFLECT))
        return;

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_COLD);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_FIRE);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_ACID);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_ELEC);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_NETH);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_WATE);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_MANA);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_PLAS);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_ICEE);

    if (int_outof(msr_ptr->r_ptr, 150))
        msr_ptr->ability_flags.reset(RF_ABILITY::MISSILE);
}

void check_high_resistances(PlayerType *player_ptr, msr_type *msr_ptr)
{
    check_nether_resistance(player_ptr, msr_ptr);
    check_lite_resistance(msr_ptr);
    check_dark_resistance(player_ptr, msr_ptr);
    if (msr_ptr->smart.has(SM::RES_FEAR))
        msr_ptr->ability_flags.reset(RF_ABILITY::SCARE);

    check_conf_resistance(msr_ptr);
    check_chaos_resistance(msr_ptr);
    if (msr_ptr->smart.has(SM::RES_DISEN) && int_outof(msr_ptr->r_ptr, 40))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_DISE);

    if (msr_ptr->smart.has(SM::RES_BLIND))
        msr_ptr->ability_flags.reset(RF_ABILITY::BLIND);

    check_nexus_resistance(msr_ptr);
    if (msr_ptr->smart.has(SM::RES_SOUND) && int_outof(msr_ptr->r_ptr, 50))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_SOUN);

    if (msr_ptr->smart.has(SM::RES_SHARD) && int_outof(msr_ptr->r_ptr, 40))
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_SHAR);

    check_reflection(msr_ptr);
    if (msr_ptr->smart.has(SM::IMM_FREE)) {
        msr_ptr->ability_flags.reset(RF_ABILITY::HOLD);
        msr_ptr->ability_flags.reset(RF_ABILITY::SLOW);
    }

    if (msr_ptr->smart.has(SM::IMM_MANA))
        msr_ptr->ability_flags.reset(RF_ABILITY::DRAIN_MANA);
}
