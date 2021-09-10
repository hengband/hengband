#include "load/load-v1-3-0.h"
#include "object-enchant/tr-types.h"
#include "system/player-type-definition.h"

void set_spells_old(player_type *player_ptr)
{
    if (player_ptr->pclass != CLASS_SMITH)
        return;

    player_ptr->magic_num1[TR_ES_ATTACK] = player_ptr->magic_num1[96];
    player_ptr->magic_num1[96] = 0;
    player_ptr->magic_num1[TR_ES_AC] = player_ptr->magic_num1[97];
    player_ptr->magic_num1[97] = 0;
}
