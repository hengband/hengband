#pragma once

#include "angband.h"
#include "files.h" // 消すかどうか検討中.

void display_one_characteristic_info(player_type *creature_ptr, TERM_LEN row, TERM_LEN col, concptr header, int flag1, all_player_flags *f, u16b mode);
