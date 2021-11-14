#pragma once

#include "system/angband.h"

extern MONSTER_IDX target_who;
extern POSITION target_col;
extern POSITION target_row;

class PlayerType;
void verify_panel(PlayerType *player_ptr);
bool target_okay(PlayerType *player_ptr);
