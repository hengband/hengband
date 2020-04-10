#pragma once

extern errr process_pref_file(player_type *creature_ptr, concptr name);
extern errr process_autopick_file(player_type *creature_ptr, concptr name);
extern errr process_histpref_file(player_type *creature_ptr, concptr name);

void auto_dump_printf(FILE *auto_dump_stream, concptr fmt, ...);
bool open_auto_dump(FILE *auto_dump_stream, concptr buf, concptr mark);
void close_auto_dump(FILE *auto_dump_stream);
