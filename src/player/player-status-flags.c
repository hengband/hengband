#include "player/player-status.h"
#include "player/player-race.h"
#include "player/player-race-types.h"
#include "realm/realm-song-numbers.h"
#include "system/monster-type-definition.h"
#include "monster-race/race-flags2.h"
#include "monster-race/monster-race.h"
#include "system/monster-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "util/bit-flags-calculator.h"


void have_kill_wall(player_type *creature_ptr)
{
    creature_ptr->kill_wall = FALSE;

    if (creature_ptr->mimic_form == MIMIC_DEMON_LORD) {
        creature_ptr->kill_wall = TRUE;
    }

    if (music_singing(creature_ptr, MUSIC_WALL)) {
        creature_ptr->kill_wall = TRUE;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (riding_r_ptr->flags2 & RF2_KILL_WALL)
            creature_ptr->kill_wall = TRUE;
    }
}

void have_pass_wall(player_type *creature_ptr)
{
    creature_ptr->pass_wall = FALSE;

    if (creature_ptr->wraith_form) {
        creature_ptr->pass_wall = TRUE;
    }

    if (creature_ptr->tim_pass_wall) {
        creature_ptr->pass_wall = TRUE;
    }

    if (creature_ptr->riding) {
        monster_type *riding_m_ptr = &creature_ptr->current_floor_ptr->m_list[creature_ptr->riding];
        monster_race *riding_r_ptr = &r_info[riding_m_ptr->r_idx];
        if (!(riding_r_ptr->flags2 & RF2_PASS_WALL))
            creature_ptr->pass_wall = FALSE;
    }
}

void have_xtra_might(player_type *creature_ptr)
{
    object_type *o_ptr;
    BIT_FLAGS flgs[TR_FLAG_SIZE];

    creature_ptr->xtra_might = FALSE;

    for (int i = INVEN_RARM; i < INVEN_TOTAL; i++) {
        o_ptr = &creature_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        object_flags(creature_ptr, o_ptr, flgs);

        if (have_flag(flgs, TR_XTRA_MIGHT))
            creature_ptr->xtra_might = TRUE;
    }
}
