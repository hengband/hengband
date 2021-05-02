#pragma once

/*
 * This seems like a pretty standard "typedef"
 */
typedef struct object_type object_type;
typedef struct player_type player_type;
typedef int (*inven_func)(player_type *creature_ptr, object_type *);

void inventory_damage(player_type *creature_ptr, inven_func typ, int perc);
