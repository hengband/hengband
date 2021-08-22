#include "load/player-attack-loader.h"
#include "load/angband-version-comparer.h"
#include "load/load-util.h"
#include "load/load-zangband.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "system/player-type-definition.h"

void rd_special_attack(player_type *creature_ptr)
{
    if (h_older_than(0, 0, 9)) {
        set_zangband_special_attack(creature_ptr);
        return;
    }

    rd_s16b(&creature_ptr->ele_attack);
    rd_u32b(&creature_ptr->special_attack);
}

void rd_special_action(player_type *creature_ptr)
{
    if (creature_ptr->special_attack & KAMAE_MASK) {
        creature_ptr->action = ACTION_KAMAE;
        return;
    }

    if (creature_ptr->special_attack & KATA_MASK)
        creature_ptr->action = ACTION_KATA;
}

void rd_special_defense(player_type *creature_ptr)
{
    if (h_older_than(0, 0, 12)) {
        set_zangband_special_defense(creature_ptr);
        return;
    }

    rd_s16b(&creature_ptr->ele_immune);
    rd_u32b(&creature_ptr->special_defense);
}

void rd_action(player_type *creature_ptr)
{
    byte tmp8u;
    rd_byte(&tmp8u);
    rd_byte(&tmp8u);
    creature_ptr->action = tmp8u;
    if (!h_older_than(0, 4, 3)) {
        set_zangband_action(creature_ptr);
    }
}
