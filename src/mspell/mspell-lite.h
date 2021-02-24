#pragma once

#include "system/angband.h"

typedef struct monster_type monster_type;
typedef struct msa_type msa_type;
typedef bool (*path_check_pf)(const player_type *, POSITION, POSITION, POSITION, POSITION);
bool adjacent_grid_check(const player_type *target_ptr, monster_type *m_ptr, POSITION *yp, POSITION *xp, int f_flag, path_check_pf path_check);
void decide_lite_range(const player_type *target_ptr, msa_type *msa_ptr);
bool decide_lite_projection(const player_type *target_ptr, msa_type *msa_ptr);
void decide_lite_area(const player_type *target_ptr, msa_type *msa_ptr);
