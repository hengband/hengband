#pragma once

#include "system/angband.h"

struct dungeon_type;
struct dun_data_type;
struct dt_type;
class PlayerType;
void gen_caverns_and_lakes(PlayerType *player_ptr, dungeon_type *dungeon_ptr, dun_data_type *dd_ptr);
void try_door(PlayerType *player_ptr, dt_type *dt_ptr, POSITION y, POSITION x);
