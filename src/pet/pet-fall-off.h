﻿#pragma once

#include "system/angband.h"

typedef struct monap_type monap_type;
class player_type;
void check_fall_off_horse(player_type *player_ptr, monap_type *monap_ptr);
bool process_fall_off_horse(player_type *player_ptr, HIT_POINT dam, bool force);
