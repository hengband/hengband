#pragma once

class FloorType;
class PlayerType;
void wipe_generate_random_floor_flags(FloorType &floor);
void clear_cave(PlayerType *player_ptr);
void generate_floor(PlayerType *player_ptr);
