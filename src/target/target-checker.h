#pragma once

#include "system/angband.h"

extern MONSTER_IDX target_who;
extern POSITION target_col;
extern POSITION target_row;

struct player_type;
void verify_panel(player_type *player_ptr);
bool target_okay(player_type *player_ptr);
