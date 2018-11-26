#include "angband.h"
#include "player-item.h"

object_type *ref_item(player_type *player_ptr, INVENTORY_IDX idx)
{
	(player_ptr); // しばらくは未使用
	return idx >= 0 ? &inventory[idx] : &o_list[0 - idx];
}