﻿#pragma once

#include "system/angband.h"

class player_type;
void wiz_lite(player_type *player_ptr, bool ninja);
void wiz_dark(player_type *player_ptr);
void map_area(player_type *player_ptr, POSITION range);
bool destroy_area(player_type *player_ptr, POSITION y1, POSITION x1, POSITION r, bool in_generate);
