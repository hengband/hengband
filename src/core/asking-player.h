#pragma once

#include "system/angband.h"

/*
 * Bit flags for control of get_check_strict()
 */
#define CHECK_OKAY_CANCEL 0x01
#define CHECK_NO_ESCAPE 0x02
#define CHECK_NO_HISTORY 0x04
#define CHECK_DEFAULT_Y 0x08

struct player_type;
bool askfor_aux(char *buf, int len, bool numpad_cursor);
bool askfor(char *buf, int len);
bool get_string(concptr prompt, char *buf, int len);
bool get_check(concptr prompt);
bool get_check_strict(player_type *player_ptr, concptr prompt, BIT_FLAGS mode);
bool get_com(concptr prompt, char *command, bool z_escape);
QUANTITY get_quantity(concptr prompt, QUANTITY max);
void pause_line(int row);
