#pragma once

class FloorType;
class PlayerType;
void wipe_generate_random_floor_flags(FloorType *floor_ptr);
void clear_cave();
void generate_floor(PlayerType *player_ptr);
