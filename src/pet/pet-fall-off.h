#pragma once

#include "system/angband.h"

typedef struct monap_type monap_type;
class PlayerType;
void check_fall_off_horse(PlayerType *player_ptr, monap_type *monap_ptr);
bool process_fall_off_horse(PlayerType *player_ptr, HIT_POINT dam, bool force);
