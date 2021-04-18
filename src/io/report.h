#pragma once

#include "system/angband.h"

extern concptr screen_dump;

#ifdef WORLD_SCORE
#include "io/files-util.h"

extern errr report_score(player_type *creature_ptr, void(*update_playtime)(void), display_player_pf display_player);
extern concptr make_screen_dump(player_type *creature_ptr);
#endif
