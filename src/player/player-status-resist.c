#include "player/player-status-resist.h"
#include "player/mimic-info-table.h"
#include "art-definition/art-sword-types.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "mutation/mutation-flag-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-weapon.h"
#include "object/object-flags.h"
#include "player/player-class.h"
#include "player/player-race-types.h"
#include "player/player-race.h"
#include "player/player-skill.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "status/element-resistance.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"

PERCENTAGE randrate(int dice, int fix, rate_calc_type_mode mode)
{
    switch (mode) {
    case CALC_RAND:
        return randint1(dice) * 100 + fix * 100;
        break;
    case CALC_AVERAGE:
        return (dice + 1) * 50 + fix * 100;
        break;
    case CALC_MIN:
        return (fix + 1) * 100;
        break;
    case CALC_MAX:
        return (dice + fix) * 100;
        break;
    default:
        return (fix + 1) * 100;
        break;
    }
}

PERCENTAGE calc_acid_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    int i;

    if (is_immune_acid(creature_ptr)) {
        return 0;
    }

    BIT_FLAGS flgs = is_vuln_acid(creature_ptr);
    for (i = 0; i < FLAG_CAUSE_MAX; i++) {
        if (flgs & (0x01 << i)) {
            if (i == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (creature_ptr->resist_acid)
        per = (per + 2) / 3;
    if (is_oppose_acid(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

PERCENTAGE calc_elec_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    int i;

    if (is_immune_elec(creature_ptr)) {
        return 0;
    }

    BIT_FLAGS flgs = is_vuln_elec(creature_ptr);
    for (i = 0; i < FLAG_CAUSE_MAX; i++) {
        if (flgs & (0x01 << i)) {
            if (i == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (creature_ptr->resist_elec)
        per = (per + 2) / 3;
    if (is_oppose_elec(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

PERCENTAGE calc_fire_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    int i;
    BIT_FLAGS flgs = is_vuln_fire(creature_ptr);
    for (i = 0; i < FLAG_CAUSE_MAX; i++) {
        if (flgs & (0x01 << i)) {
            if (i == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    /* Resist the damage */
    if (creature_ptr->resist_fire)
        per = (per + 2) / 3;
    if (is_oppose_fire(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

PERCENTAGE calc_cold_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    int i;
    BIT_FLAGS flgs = is_vuln_cold(creature_ptr);
    for (i = 0; i < FLAG_CAUSE_MAX; i++) {
        if (flgs & (0x01 << i)) {
            if (i == FLAG_CAUSE_MUTATION) {
                per *= 2;
            } else {
                per += per / 3;
            }
        }
    }

    if (creature_ptr->resist_cold)
        per = (per + 2) / 3;
    if (is_oppose_cold(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

PERCENTAGE calc_pois_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    if (creature_ptr->resist_pois)
        per = (per + 2) / 3;
    if (is_oppose_pois(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

PERCENTAGE calc_nuke_damage_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    if (creature_ptr->resist_pois)
        per = (per + 2) / 3;
    if (is_oppose_pois(creature_ptr))
        per = (per + 2) / 3;

    return per;
}

PERCENTAGE calc_deathray_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (mode); // unused
    if (creature_ptr->mimic_form) {
        if (mimic_info[creature_ptr->mimic_form].MIMIC_FLAGS & MIMIC_IS_NONLIVING) {
            return 0;
        }
    }

    switch (creature_ptr->prace) {
    case RACE_GOLEM:
    case RACE_SKELETON:
    case RACE_ZOMBIE:
    case RACE_VAMPIRE:
    case RACE_BALROG:
    case RACE_SPECTRE:
        return 0;
        break;
    }

    return 100;
}

PERCENTAGE calc_lite_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;
    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || (creature_ptr->mimic_form == MIMIC_VAMPIRE)) {
        per *= 2;
    } else if (is_specific_player_race(creature_ptr, RACE_S_FAIRY)) {
        per = per * 4 / 3;
    }

    if (creature_ptr->resist_lite) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    if (creature_ptr->wraith_form)
        per *= 2;

    return per;
}

PERCENTAGE calc_dark_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || (creature_ptr->mimic_form == MIMIC_VAMPIRE) || creature_ptr->wraith_form) {
        return 0;
    }

    if (creature_ptr->resist_dark) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_shards_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (creature_ptr->resist_shard) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_sound_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (creature_ptr->resist_sound) {
        per *= 500;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_conf_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (creature_ptr->resist_conf) {
        per *= 500;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_chaos_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (creature_ptr->resist_chaos) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_disenchant_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (creature_ptr->resist_disen) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_nexus_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (creature_ptr->resist_disen) {
        per *= 600;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_rocket_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (mode); // unused
    PERCENTAGE per = 100;

    if (creature_ptr->resist_shard) {
        per /= 2;
    }

    return per;
}

PERCENTAGE calc_nether_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    PERCENTAGE per = 100;

    if (creature_ptr->resist_neth) {
        if (!is_specific_player_race(creature_ptr, RACE_SPECTRE))
            per *= 6;
        per *= 100;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_time_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (mode); // unused
    PERCENTAGE per = 100;

    if (creature_ptr->resist_time) {
        per *= 400;
        per /= randrate(4, 7, mode);
    }

    return per;
}

PERCENTAGE calc_holy_fire_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (mode); // unused
    PERCENTAGE per = 100;
    if (creature_ptr->align > 10)
        per /= 2;
    else if (creature_ptr->align < -10)
        per *= 2;
    return per;
}

PERCENTAGE calc_hell_fire_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (mode); // unused
    PERCENTAGE per = 100;
    if (creature_ptr->align > 10)
        per *= 2;
    return per;
}

PERCENTAGE calc_gravity_damage_rate(player_type *creature_ptr, rate_calc_type_mode mode)
{
    (mode); // unused
    PERCENTAGE per = 100;
    if (creature_ptr->levitation) {
        per = (per * 2) / 3;
    }
    return per;
}
