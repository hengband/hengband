#pragma once

#include "system/angband.h"
#include <string>

class FloorType;
extern FloorType floor_info;

class PlayerType;
void update_smell(FloorType *floor_ptr, PlayerType *player_ptr);
void forget_flow(FloorType *floor_ptr);
void wipe_o_list(FloorType *floor_ptr);
void scatter(PlayerType *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
std::string map_name(PlayerType *player_ptr);
