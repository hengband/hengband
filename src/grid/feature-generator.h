#pragma once

#include "system/angband.h"

typedef struct dungeon_type dungeon_type;
typedef struct dun_data_type dun_data_type;
typedef struct dt_type dt_type;
typedef struct player_type player_type;
void gen_caverns_and_lakes(player_type *owner_ptr, dungeon_type *dungeon_ptr, dun_data_type *dd_ptr);
bool has_river_flag(dungeon_type *dungeon_ptr);
void try_door(player_type *player_ptr, dt_type *dt_ptr, POSITION y, POSITION x);
