#pragma once

#include "system/angband.h"

extern MONSTER_IDX target_who;
extern POSITION target_col;
extern POSITION target_row;

typedef enum target_type target_type;
bool target_set(player_type *creature_ptr, target_type mode);

void verify_panel(player_type *creature_ptr);
bool target_okay(player_type *creature_ptr);
bool get_aim_dir(player_type *creature_ptr, DIRECTION *dp);
bool get_direction(player_type *creature_ptr, DIRECTION *dp, bool allow_under, bool with_steed);
bool get_rep_dir(player_type *creature_ptr, DIRECTION *dp, bool under);
bool tgt_pt(player_type *creature_ptr, POSITION *x, POSITION *y);
