#include "mspell/element-resistance-checker.h"
#include "game-option/birth-options.h"
#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "mspell/smart-mspell-util.h"
#include "player/player-status-flags.h"
#include "status/element-resistance.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

void add_cheat_remove_flags_element(player_type *player_ptr, msr_type *msr_ptr)
{
    if (has_resist_acid(player_ptr))
        msr_ptr->smart.set(SM::RES_ACID);

    if (is_oppose_acid(player_ptr))
        msr_ptr->smart.set(SM::OPP_ACID);

    if (has_immune_acid(player_ptr))
        msr_ptr->smart.set(SM::IMM_ACID);

    if (has_resist_elec(player_ptr))
        msr_ptr->smart.set(SM::RES_ELEC);

    if (is_oppose_elec(player_ptr))
        msr_ptr->smart.set(SM::OPP_ELEC);

    if (has_immune_elec(player_ptr))
        msr_ptr->smart.set(SM::IMM_ELEC);

    if (has_resist_fire(player_ptr))
        msr_ptr->smart.set(SM::RES_FIRE);

    if (is_oppose_fire(player_ptr))
        msr_ptr->smart.set(SM::OPP_FIRE);

    if (has_immune_fire(player_ptr))
        msr_ptr->smart.set(SM::IMM_FIRE);

    if (has_resist_cold(player_ptr))
        msr_ptr->smart.set(SM::RES_COLD);

    if (is_oppose_cold(player_ptr))
        msr_ptr->smart.set(SM::OPP_COLD);

    if (has_immune_cold(player_ptr))
        msr_ptr->smart.set(SM::IMM_COLD);

    if (has_resist_pois(player_ptr))
        msr_ptr->smart.set(SM::RES_POIS);

    if (is_oppose_pois(player_ptr))
        msr_ptr->smart.set(SM::OPP_POIS);
}

static void check_acid_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has(SM::IMM_ACID)) {
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_ACID);
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_ACID);
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_ACID);
        return;
    }

    if (msr_ptr->smart.has_all_of({SM::OPP_ACID, SM::RES_ACID})) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_ACID);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_ACID);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_ACID);

        return;
    }

    if (msr_ptr->smart.has_any_of({SM::OPP_ACID, SM::RES_ACID})) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_ACID);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_ACID);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_ACID);
    }
}

static void check_elec_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has(SM::IMM_ELEC)) {
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_ELEC);
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_ELEC);
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_ELEC);
        return;
    }

    if (msr_ptr->smart.has_all_of({SM::OPP_ELEC, SM::RES_ELEC})) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_ELEC);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_ELEC);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_ELEC);

        return;
    }

    if (msr_ptr->smart.has_any_of({SM::OPP_ELEC, SM::RES_ELEC})) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_ELEC);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_ELEC);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_ELEC);
    }
}

static void check_fire_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has(SM::IMM_FIRE)) {
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_FIRE);
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_FIRE);
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_FIRE);
        return;
    }

    if (msr_ptr->smart.has_all_of({SM::OPP_FIRE, SM::RES_FIRE})) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_FIRE);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_FIRE);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_FIRE);

        return;
    }

    if (msr_ptr->smart.has_any_of({SM::OPP_FIRE, SM::RES_FIRE})) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_FIRE);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_FIRE);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_FIRE);
    }
}

static void check_cold_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has(SM::IMM_COLD)) {
        msr_ptr->ability_flags.reset(RF_ABILITY::BR_COLD);
        msr_ptr->ability_flags.reset(RF_ABILITY::BA_COLD);
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_COLD);
        msr_ptr->ability_flags.reset(RF_ABILITY::BO_ICEE);
        return;
    }

    if (msr_ptr->smart.has_all_of({SM::OPP_COLD, SM::RES_COLD})) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_ICEE);

        return;
    }

    if (msr_ptr->smart.has_any_of({SM::OPP_COLD, SM::RES_COLD})) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_COLD);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_COLD);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_COLD);

        if (int_outof(msr_ptr->r_ptr, 20))
            msr_ptr->ability_flags.reset(RF_ABILITY::BO_ICEE);
    }
}

static void check_pois_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart.has_all_of({SM::OPP_POIS, SM::RES_POIS})) {
        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_POIS);

        if (int_outof(msr_ptr->r_ptr, 80))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_POIS);

        if (int_outof(msr_ptr->r_ptr, 60))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_NUKE);

        if (int_outof(msr_ptr->r_ptr, 60))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_NUKE);

        return;
    }

    if (msr_ptr->smart.has_any_of({SM::OPP_POIS, SM::RES_POIS})) {
        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BR_POIS);

        if (int_outof(msr_ptr->r_ptr, 30))
            msr_ptr->ability_flags.reset(RF_ABILITY::BA_POIS);
    }
}

void check_element_resistance(msr_type *msr_ptr)
{
    check_acid_resistance(msr_ptr);
    check_elec_resistance(msr_ptr);
    check_fire_resistance(msr_ptr);
    check_cold_resistance(msr_ptr);
    check_pois_resistance(msr_ptr);
}
