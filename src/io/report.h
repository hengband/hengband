#pragma once

#include "system/angband.h"

extern concptr screen_dump;

#ifdef WORLD_SCORE
#include "io/files-util.h"

class PlayerType;
bool report_score(PlayerType *player_ptr, display_player_pf display_player);
concptr make_screen_dump(PlayerType *player_ptr);
#endif
