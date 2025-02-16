#pragma once

#include "util/point-2d.h"

class DungeonDefinition;
class DungeonData;
struct dt_type;
class PlayerType;
void gen_caverns_and_lakes(PlayerType *player_ptr, const DungeonDefinition &dungeon, DungeonData *dd_ptr);
void try_door(PlayerType *player_ptr, dt_type *dt_ptr, const Pos2D &pos);
