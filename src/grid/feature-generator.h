#pragma once

#include "system/angband.h"

struct dungeon_type;
class DungeonData;
struct dt_type;
class PlayerType;
void gen_caverns_and_lakes(PlayerType *player_ptr, dungeon_type *dungeon_ptr, DungeonData *dd_ptr);
void try_door(PlayerType *player_ptr, dt_type *dt_ptr, POSITION y, POSITION x);
