#pragma once

#include "util/point-2d.h"

class Direction;
class PlayerType;
void lite_room(PlayerType *player_ptr, const Pos2D &pos_start);
bool starlight(PlayerType *player_ptr, bool magic);
void unlite_room(PlayerType *player_ptr, const Pos2D &pos_start);
bool lite_area(PlayerType *player_ptr, int dam, int rad);
bool unlite_area(PlayerType *player_ptr, int dam, int rad);
bool lite_line(PlayerType *player_ptr, const Direction &dir, int dam);
