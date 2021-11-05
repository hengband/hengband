#pragma once

struct floor_type;
class PlayerType;
void wipe_generate_random_floor_flags(floor_type *floor_ptr);
void clear_cave(PlayerType *player_ptr);
void generate_floor(PlayerType *player_ptr);
