#include "mspell/element-resistance-checker.h"
#include "game-option/birth-options.h"
#include "monster-race/race-flags-ability1.h"
#include "monster-race/race-flags4.h"
#include "monster/smart-learn-types.h"
#include "mspell/smart-mspell-util.h"
#include "player/player-status-flags.h"
#include "status/element-resistance.h"
#include "util/bit-flags-calculator.h"

void add_cheat_remove_flags_element(player_type *target_ptr, msr_type *msr_ptr)
{
    if (has_resist_acid(target_ptr))
        set_bits(msr_ptr->smart, SM_RES_ACID);

    if (is_oppose_acid(target_ptr))
        set_bits(msr_ptr->smart, SM_OPP_ACID);

    if (has_immune_acid(target_ptr))
        set_bits(msr_ptr->smart, SM_IMM_ACID);

    if (has_resist_elec(target_ptr))
        set_bits(msr_ptr->smart, SM_RES_ELEC);

    if (is_oppose_elec(target_ptr))
        set_bits(msr_ptr->smart, SM_OPP_ELEC);

    if (has_immune_elec(target_ptr))
        set_bits(msr_ptr->smart, SM_IMM_ELEC);

    if (has_resist_fire(target_ptr))
        set_bits(msr_ptr->smart, SM_RES_FIRE);

    if (is_oppose_fire(target_ptr))
        set_bits(msr_ptr->smart, SM_OPP_FIRE);

    if (has_immune_fire(target_ptr))
        set_bits(msr_ptr->smart, SM_IMM_FIRE);

    if (has_resist_cold(target_ptr))
        set_bits(msr_ptr->smart, SM_RES_COLD);

    if (is_oppose_cold(target_ptr))
        set_bits(msr_ptr->smart, SM_OPP_COLD);

    if (has_immune_cold(target_ptr))
        set_bits(msr_ptr->smart, SM_IMM_COLD);

    if (has_resist_pois(target_ptr))
        set_bits(msr_ptr->smart, SM_RES_POIS);

    if (is_oppose_pois(target_ptr))
        set_bits(msr_ptr->smart, SM_OPP_POIS);
}

static void check_acid_resistance(msr_type *msr_ptr)
{
    if (any_bits(msr_ptr->smart, SM_IMM_ACID)) {
        reset_bits(msr_ptr->f4, RF4_BR_ACID);
        reset_bits(msr_ptr->f5, RF5_BA_ACID);
        reset_bits(msr_ptr->f5, RF5_BO_ACID);
        return;
    }

    if (all_bits(msr_ptr->smart, (SM_OPP_ACID | SM_RES_ACID))) {
        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f4, RF4_BR_ACID);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BA_ACID);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BO_ACID);

        return;
    }

    if (any_bits(msr_ptr->smart, (SM_OPP_ACID | SM_RES_ACID))) {
        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f4, RF4_BR_ACID);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BA_ACID);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BO_ACID);
    }
}

static void check_elec_resistance(msr_type *msr_ptr)
{
    if (any_bits(msr_ptr->smart, SM_IMM_ELEC)) {
        reset_bits(msr_ptr->f4, RF4_BR_ELEC);
        reset_bits(msr_ptr->f5, RF5_BA_ELEC);
        reset_bits(msr_ptr->f5, RF5_BO_ELEC);
        return;
    }

    if (all_bits(msr_ptr->smart, (SM_OPP_ELEC | SM_RES_ELEC))) {
        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f4, RF4_BR_ELEC);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BA_ELEC);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BO_ELEC);

        return;
    }

    if (any_bits(msr_ptr->smart, (SM_OPP_ELEC | SM_RES_ELEC))) {
        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f4, RF4_BR_ELEC);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BA_ELEC);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BO_ELEC);
    }
}

static void check_fire_resistance(msr_type *msr_ptr)
{
    if (any_bits(msr_ptr->smart, SM_IMM_FIRE)) {
        reset_bits(msr_ptr->f4, RF4_BR_FIRE);
        reset_bits(msr_ptr->f5, RF5_BA_FIRE);
        reset_bits(msr_ptr->f5, RF5_BO_FIRE);
        return;
    }

    if (all_bits(msr_ptr->smart, (SM_OPP_FIRE | SM_RES_FIRE))) {
        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f4, RF4_BR_FIRE);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BA_FIRE);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BO_FIRE);

        return;
    }

    if (any_bits(msr_ptr->smart, (SM_OPP_FIRE | SM_RES_FIRE))) {
        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f4, RF4_BR_FIRE);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BA_FIRE);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BO_FIRE);
    }
}

static void check_cold_resistance(msr_type *msr_ptr)
{
    if (any_bits(msr_ptr->smart, SM_IMM_COLD)) {
        reset_bits(msr_ptr->f4, RF4_BR_COLD);
        reset_bits(msr_ptr->f5, RF5_BA_COLD);
        reset_bits(msr_ptr->f5, RF5_BO_COLD);
        reset_bits(msr_ptr->f5, RF5_BO_ICEE);
        return;
    }

    if (all_bits(msr_ptr->smart, (SM_OPP_COLD | SM_RES_COLD))) {
        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f4, RF4_BR_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BA_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BO_COLD);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BO_ICEE);

        return;
    }

    if (any_bits(msr_ptr->smart, (SM_OPP_COLD | SM_RES_COLD))) {
        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f4, RF4_BR_COLD);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BA_COLD);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BO_COLD);

        if (int_outof(msr_ptr->r_ptr, 20))
            reset_bits(msr_ptr->f5, RF5_BO_ICEE);
    }
}

static void check_pois_resistance(msr_type *msr_ptr)
{
    if (all_bits(msr_ptr->smart, (SM_OPP_POIS | SM_RES_POIS))) {
        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f4, RF4_BR_POIS);

        if (int_outof(msr_ptr->r_ptr, 80))
            reset_bits(msr_ptr->f5, RF5_BA_POIS);

        if (int_outof(msr_ptr->r_ptr, 60))
            reset_bits(msr_ptr->f4, RF4_BA_NUKE);

        if (int_outof(msr_ptr->r_ptr, 60))
            reset_bits(msr_ptr->f4, RF4_BR_NUKE);

        return;
    }

    if (any_bits(msr_ptr->smart, (SM_OPP_POIS | SM_RES_POIS))) {
        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f4, RF4_BR_POIS);

        if (int_outof(msr_ptr->r_ptr, 30))
            reset_bits(msr_ptr->f5, RF5_BA_POIS);
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
