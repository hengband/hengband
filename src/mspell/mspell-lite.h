#pragma once

#include "system/angband.h"

typedef struct monster_type monster_type;
typedef struct msa_type msa_type;
typedef bool (*path_check_pf)(player_type *, POSITION, POSITION, POSITION, POSITION);
bool adjacent_grid_check(player_type *target_ptr, monster_type *m_ptr, POSITION *yp, POSITION *xp, int f_flag, path_check_pf path_check);
void decide_lite_range(player_type *target_ptr, msa_type *msa_ptr);
