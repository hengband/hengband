#pragma once

#include "system/angband.h"

bool place_quest_monsters(player_type *creature_ptr);
void wipe_generate_random_floor_flags(floor_type *floor_ptr);
void clear_cave(player_type *player_ptr);
void generate_floor(player_type *player_ptr);

typedef struct dt_type dt_type;
bool build_tunnel(player_type *player_ptr, dt_type *dt_ptr, POSITION row1, POSITION col1, POSITION row2, POSITION col2);
bool build_tunnel2(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff);
