#pragma once

extern concptr screen_dump;

#ifdef WORLD_SCORE
#include "io/files.h"

extern errr report_score(player_type *creature_ptr, void(*update_playtime)(void), display_player_pf display_player, map_name_pf map_name);
extern concptr make_screen_dump(player_type *creature_ptr, void(*process_autopick_file_command)(char*));
#endif
