#pragma once

typedef struct floor_type floor_type;
struct player_type;
void wipe_generate_random_floor_flags(floor_type *floor_ptr);
void clear_cave(player_type *player_ptr);
void generate_floor(player_type *player_ptr);
