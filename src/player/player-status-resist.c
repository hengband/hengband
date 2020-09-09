#include "player/player-status-flags.h"
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
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "realm/realm-song-numbers.h"
#include "realm/realm-types.h"
#include "spell-realm/spells-hex.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/quarks.h"
#include "util/string-processor.h"

PERCENTAGE calc_vuln_acid_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    int i;
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
    return per;
}

PERCENTAGE calc_vuln_elec_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    int i;
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
    return per;
}

PERCENTAGE calc_vuln_fire_rate(player_type *creature_ptr)
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
    return per;
}

PERCENTAGE calc_vuln_cold_rate(player_type *creature_ptr)
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
    return per;
}

PERCENTAGE calc_vuln_lite_rate(player_type *creature_ptr)
{
    PERCENTAGE per = 100;
    if (is_specific_player_race(creature_ptr, RACE_VAMPIRE) || (creature_ptr->mimic_form == MIMIC_VAMPIRE)) {
        per *= 2;
    } else if (is_specific_player_race(creature_ptr, RACE_S_FAIRY)) {
        per = per * 4 / 3;
    }

    if (creature_ptr->wraith_form)
        per *= 2;

    return per;
}
