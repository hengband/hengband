#pragma once

#include "system/angband.h"

void generate_room_floor(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2, int light);
void generate_fill_perm_bold(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
bool generate_rooms(player_type *player_ptr);
