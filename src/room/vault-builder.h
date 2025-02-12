#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

class FloorType;
class PlayerType;
void vault_monsters(PlayerType *player_ptr, POSITION y1, POSITION x1, int num);
void vault_objects(PlayerType *player_ptr, POSITION y, POSITION x, int num);
void vault_traps(FloorType &floor, const Pos2D &pos_center, const Pos2DVec &distribution, int num);
