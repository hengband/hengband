#pragma once

#include "system/angband.h"
#include <string>

class PlayerType;
struct saved_floor_type;
void init_saved_floors(bool force);
void clear_saved_floor_files(PlayerType *player_ptr);
saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id);
std::string get_saved_floor_name(int savefile_id);
void kill_saved_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr);
FLOOR_IDX get_unused_floor_id(PlayerType *player_ptr);
void precalc_cur_num_of_pet();
extern FLOOR_IDX max_floor_id;
