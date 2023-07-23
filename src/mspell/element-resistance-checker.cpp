#include "mspell/element-resistance-checker.h"
#include "game-option/birth-options.h"
#include "monster-race/race-ability-flags.h"
#include "monster/smart-learn-types.h"
#include "mspell/smart-mspell-util.h"
#include "player/player-status-flags.h"
#include "status/element-resistance.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

void add_cheat_remove_flags_element(PlayerType *player_ptr, msr_type *msr_ptr)
{
    if (has_resist_acid(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_ACID);
    }

    if (is_oppose_acid(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::OPP_ACID);
    }

    if (has_immune_acid(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::IMM_ACID);
    }

    if (has_resist_elec(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_ELEC);
    }

    if (is_oppose_elec(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::OPP_ELEC);
    }

    if (has_immune_elec(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::IMM_ELEC);
    }

    if (has_resist_fire(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_FIRE);
    }

    if (is_oppose_fire(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::OPP_FIRE);
    }

    if (has_immune_fire(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::IMM_FIRE);
    }

    if (has_resist_cold(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_COLD);
    }

    if (is_oppose_cold(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::OPP_COLD);
    }

    if (has_immune_cold(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::IMM_COLD);
    }

    if (has_resist_pois(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::RES_POIS);
    }

    if (is_oppose_pois(player_ptr)) {
        msr_ptr->smart_flags.set(MonsterSmartLearnType::OPP_POIS);
    }
}

static void check_acid_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::IMM_ACID)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_ACID);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_ACID);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ACID);
        return;
    }

    if (msr_ptr->smart_flags.has_all_of({ MonsterSmartLearnType::OPP_ACID, MonsterSmartLearnType::RES_ACID })) {
        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_ACID);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_ACID);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ACID);
        }

        return;
    }

    if (msr_ptr->smart_flags.has_any_of({ MonsterSmartLearnType::OPP_ACID, MonsterSmartLearnType::RES_ACID })) {
        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_ACID);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_ACID);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ACID);
        }
    }
}

static void check_elec_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::IMM_ELEC)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_ELEC);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_ELEC);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ELEC);
        return;
    }

    if (msr_ptr->smart_flags.has_all_of({ MonsterSmartLearnType::OPP_ELEC, MonsterSmartLearnType::RES_ELEC })) {
        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_ELEC);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_ELEC);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ELEC);
        }

        return;
    }

    if (msr_ptr->smart_flags.has_any_of({ MonsterSmartLearnType::OPP_ELEC, MonsterSmartLearnType::RES_ELEC })) {
        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_ELEC);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_ELEC);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ELEC);
        }
    }
}

static void check_fire_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::IMM_FIRE)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_FIRE);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_FIRE);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_FIRE);
        return;
    }

    if (msr_ptr->smart_flags.has_all_of({ MonsterSmartLearnType::OPP_FIRE, MonsterSmartLearnType::RES_FIRE })) {
        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_FIRE);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_FIRE);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_FIRE);
        }

        return;
    }

    if (msr_ptr->smart_flags.has_any_of({ MonsterSmartLearnType::OPP_FIRE, MonsterSmartLearnType::RES_FIRE })) {
        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_FIRE);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_FIRE);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_FIRE);
        }
    }
}

static void check_cold_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has(MonsterSmartLearnType::IMM_COLD)) {
        msr_ptr->ability_flags.reset(MonsterAbilityType::BR_COLD);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BA_COLD);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_COLD);
        msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ICEE);
        return;
    }

    if (msr_ptr->smart_flags.has_all_of({ MonsterSmartLearnType::OPP_COLD, MonsterSmartLearnType::RES_COLD })) {
        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_COLD);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_COLD);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_COLD);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ICEE);
        }

        return;
    }

    if (msr_ptr->smart_flags.has_any_of({ MonsterSmartLearnType::OPP_COLD, MonsterSmartLearnType::RES_COLD })) {
        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_COLD);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_COLD);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_COLD);
        }

        if (int_outof(msr_ptr->r_ptr, 20)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BO_ICEE);
        }
    }
}

static void check_pois_resistance(msr_type *msr_ptr)
{
    if (msr_ptr->smart_flags.has_all_of({ MonsterSmartLearnType::OPP_POIS, MonsterSmartLearnType::RES_POIS })) {
        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_POIS);
        }

        if (int_outof(msr_ptr->r_ptr, 80)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_POIS);
        }

        if (int_outof(msr_ptr->r_ptr, 60)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_NUKE);
        }

        if (int_outof(msr_ptr->r_ptr, 60)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_NUKE);
        }

        return;
    }

    if (msr_ptr->smart_flags.has_any_of({ MonsterSmartLearnType::OPP_POIS, MonsterSmartLearnType::RES_POIS })) {
        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BR_POIS);
        }

        if (int_outof(msr_ptr->r_ptr, 30)) {
            msr_ptr->ability_flags.reset(MonsterAbilityType::BA_POIS);
        }
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
