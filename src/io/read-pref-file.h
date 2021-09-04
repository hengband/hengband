#pragma once

#include "system/angband.h"

extern char auto_dump_header[];
extern char auto_dump_footer[];

struct player_type;
errr process_pref_file(player_type *creature_ptr, concptr name, bool only_user_dir = false);
errr process_autopick_file(player_type *creature_ptr, concptr name);
errr process_histpref_file(player_type *creature_ptr, concptr name);
bool read_histpref(player_type *creature_ptr);

void auto_dump_printf(FILE *auto_dump_stream, concptr fmt, ...);
bool open_auto_dump(FILE **fpp, concptr buf, concptr mark);
void close_auto_dump(FILE **fpp, concptr auto_dump_mark);

void load_all_pref_files(player_type* player_ptr);
