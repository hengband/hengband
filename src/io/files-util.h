#pragma once

#include "system/angband.h"

extern char savefile[1024];
extern char savefile_base[40];
extern char debug_savefile[1024];

extern concptr ANGBAND_DIR;
extern concptr ANGBAND_DIR_APEX;
extern concptr ANGBAND_DIR_BONE;
extern concptr ANGBAND_DIR_DATA;
extern concptr ANGBAND_DIR_EDIT;
extern concptr ANGBAND_DIR_SCRIPT;
extern concptr ANGBAND_DIR_FILE;
extern concptr ANGBAND_DIR_HELP;
extern concptr ANGBAND_DIR_INFO;
extern concptr ANGBAND_DIR_PREF;
extern concptr ANGBAND_DIR_SAVE;
extern concptr ANGBAND_DIR_DEBUG_SAVE;
extern concptr ANGBAND_DIR_USER;
extern concptr ANGBAND_DIR_XTRA;

struct player_type;
typedef void (*display_player_pf)(player_type *, int);
typedef void(*update_playtime_pf)(void);

extern errr file_character(player_type *creature_ptr, concptr name, display_player_pf display_player);
extern errr get_rnd_line(concptr file_name, int entry, char *output);
void read_dead_file(char* buf, size_t buf_size);

#ifdef JP
extern errr get_rnd_line_jonly(concptr file_name, int entry, char *output, int count);
#endif
extern errr counts_write(player_type *creature_ptr, int where, uint32_t count);
extern uint32_t counts_read(player_type *creature_ptr, int where);
