#pragma once

extern concptr screen_dump;

#ifdef WORLD_SCORE
/* report.c */
extern errr report_score(player_type *creature_ptr);
extern concptr make_screen_dump(player_type *creature_ptr);
#endif
