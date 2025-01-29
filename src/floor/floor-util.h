#pragma once

#include "util/point-2d.h"
#include <cstdint>
#include <string>

class FloorType;
extern FloorType floor_info;

class PlayerType;
void update_smell(FloorType &floor, const Pos2D &p_pos);
void forget_flow(FloorType *floor_ptr);
void wipe_o_list(FloorType *floor_ptr);
Pos2D scatter(PlayerType *player_ptr, const Pos2D &pos, int d, uint32_t mode);
std::string map_name(PlayerType *player_ptr);
