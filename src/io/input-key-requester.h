#pragma once

#include "system/angband.h"
#include "game-option/keymap-directory-getter.h"

extern concptr keymap_act[KEYMAP_MODES][256];

extern bool use_menu;

extern COMMAND_CODE command_cmd;
extern COMMAND_ARG command_arg;
extern int16_t command_rep;
extern DIRECTION command_dir;
extern int16_t command_see;
extern TERM_LEN command_gap;
extern int16_t command_wrk;
extern int16_t command_new;

class PlayerType;
void request_command(PlayerType *player_ptr, int shopping);
