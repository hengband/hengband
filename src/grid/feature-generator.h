#pragma once

#include "system/angband.h"

struct dungeon_type;
struct dun_data_type;
struct dt_type;
struct player_type;
void gen_caverns_and_lakes(player_type *player_ptr, dungeon_type *dungeon_ptr, dun_data_type *dd_ptr);
bool has_river_flag(dungeon_type *dungeon_ptr);
void try_door(player_type *player_ptr, dt_type *dt_ptr, POSITION y, POSITION x);
