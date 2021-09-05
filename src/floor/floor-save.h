#pragma once

#include "system/angband.h"

struct player_type;
struct saved_floor_type;
void init_saved_floors(player_type *creature_ptr, bool force);
void clear_saved_floor_files(player_type *creature_ptr);
saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id);
void kill_saved_floor(player_type *creature_ptr, saved_floor_type *sf_ptr);
FLOOR_IDX get_new_floor_id(player_type *creature_ptr);
void precalc_cur_num_of_pet(player_type *creature_ptr);
extern FLOOR_IDX max_floor_id;
