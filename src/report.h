#pragma once

extern concptr screen_dump;

#ifdef WORLD_SCORE
/* report.c */
extern errr report_score(void);
extern concptr make_screen_dump(void);
#endif
