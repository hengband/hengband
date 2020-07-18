#include "load/load-v1-3-0.h"
#include "object-enchant/tr-types.h"

void set_spells_old(player_type *creature_ptr)
{
    if (creature_ptr->pclass != CLASS_SMITH)
        return;

    creature_ptr->magic_num1[TR_ES_ATTACK] = creature_ptr->magic_num1[96];
    creature_ptr->magic_num1[96] = 0;
    creature_ptr->magic_num1[TR_ES_AC] = creature_ptr->magic_num1[97];
    creature_ptr->magic_num1[97] = 0;
}
