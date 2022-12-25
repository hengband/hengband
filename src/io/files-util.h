#pragma once

#include "system/angband.h"
#include <optional>
#include <string>

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

class PlayerType;
typedef void (*update_playtime_pf)(void);

errr file_character(PlayerType *player_ptr, concptr name);
std::optional<std::string> get_random_line(concptr file_name, int entry);
void read_dead_file(char *buf, size_t buf_size);

#ifdef JP
std::optional<std::string> get_random_line_ja_only(concptr file_name, int entry, int count);
#endif
errr counts_write(PlayerType *player_ptr, int where, uint32_t count);
uint32_t counts_read(PlayerType *player_ptr, int where);
