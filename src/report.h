#pragma once

extern concptr screen_dump;

#ifdef WORLD_SCORE
extern errr report_score(player_type *creature_ptr, void(*update_playtime)(void));
extern concptr make_screen_dump(player_type *creature_ptr);
#endif
