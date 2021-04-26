#pragma once

#include "system/angband.h"
#include "game-option/keymap-directory-getter.h"

extern concptr keymap_act[KEYMAP_MODES][256];

extern bool use_menu;

extern COMMAND_CODE command_cmd;
extern COMMAND_ARG command_arg;
extern s16b command_rep;
extern DIRECTION command_dir;
extern s16b command_see;
extern TERM_LEN command_gap;
extern s16b command_wrk;
extern s16b command_new;

typedef struct player_type player_type;
void request_command(player_type *player_ptr, int shopping);
