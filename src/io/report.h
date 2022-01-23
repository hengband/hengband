#pragma once

#include "system/angband.h"

extern concptr screen_dump;

#ifdef WORLD_SCORE

class PlayerType;
bool report_score(PlayerType *player_ptr);
concptr make_screen_dump(PlayerType *player_ptr);
#endif
