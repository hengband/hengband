#include "system/angband.h"
#include "mind/racial-mirror-master.h"
#include "world/world.h"

/*
 * @brief Multishadow effects is determined by turn
 */
bool check_multishadow(player_type *creature_ptr)
{
    return (creature_ptr->multishadow != 0) && ((current_world_ptr->game_turn & 1) != 0);
}
