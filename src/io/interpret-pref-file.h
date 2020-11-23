#pragma once

#include "system/angband.h"

#define HISTPREF_LIMIT 1024

extern char *histpref_buf;

errr interpret_pref_file(player_type *creature_ptr, char *buf);
void add_history_from_pref_line(concptr t);
