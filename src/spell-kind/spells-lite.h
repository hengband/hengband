#pragma once

#include "system/angband.h"

class PlayerType;
void lite_room(PlayerType *player_ptr, POSITION y1, POSITION x1);
bool starlight(PlayerType *player_ptr, bool magic);
void unlite_room(PlayerType *player_ptr, POSITION y1, POSITION x1);
bool lite_area(PlayerType *player_ptr, int dam, POSITION rad);
bool unlite_area(PlayerType *player_ptr, int dam, POSITION rad);
bool lite_line(PlayerType *player_ptr, DIRECTION dir, int dam);
