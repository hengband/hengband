#include "mspell/high-resistance-checker.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags-ability2.h"
#include "monster-race/race-flags4.h"
#include "monster/smart-learn-types.h"
#include "mspell/smart-mspell-util.h"
#include "player/player-race.h"
#include "player/player-status-flags.h"

void add_cheat_remove_flags_others(player_type *target_ptr, msr_type *msr_ptr)
{
    if (has_resist_neth(target_ptr))
        msr_ptr->smart |= SM_RES_NETH;

    if (has_resist_lite(target_ptr))
        msr_ptr->smart |= SM_RES_LITE;

    if (has_resist_dark(target_ptr))
        msr_ptr->smart |= SM_RES_DARK;

    if (has_resist_fear(target_ptr))
        msr_ptr->smart |= SM_RES_FEAR;

    if (has_resist_conf(target_ptr))
        msr_ptr->smart |= SM_RES_CONF;

    if (has_resist_chaos(target_ptr))
        msr_ptr->smart |= SM_RES_CHAOS;

    if (has_resist_disen(target_ptr))
        msr_ptr->smart |= SM_RES_DISEN;

    if (has_resist_blind(target_ptr))
        msr_ptr->smart |= SM_RES_BLIND;

    if (has_resist_nexus(target_ptr))
        msr_ptr->smart |= SM_RES_NEXUS;

    if (has_resist_sound(target_ptr))
        msr_ptr->smart |= SM_RES_SOUND;

    if (has_resist_shard(target_ptr))
        msr_ptr->smart |= SM_RES_SHARD;

    if (has_reflect(target_ptr))
        msr_ptr->smart |= SM_IMM_REFLECT;

    if (target_ptr->free_act)
        msr_ptr->smart |= SM_IMM_FREE;

    if (!target_ptr->msp)
        msr_ptr->smart |= SM_IMM_MANA;
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
