#pragma once

#include "system/angband.h"

struct floor_type;
extern floor_type floor_info;

class PlayerType;
void update_smell(floor_type *floor_ptr, PlayerType *player_ptr);
void forget_flow(floor_type *floor_ptr);
void wipe_o_list(floor_type *floor_ptr);
void scatter(PlayerType *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode);
concptr map_name(PlayerType *player_ptr);
