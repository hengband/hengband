#pragma once

#include "system/angband.h"

struct monster_type;
typedef struct msa_type msa_type;
struct player_type;
typedef bool (*path_check_pf)(player_type *, POSITION, POSITION, POSITION, POSITION);
enum class FF;
bool adjacent_grid_check(player_type *target_ptr, monster_type *m_ptr, POSITION *yp, POSITION *xp, FF f_flag, path_check_pf path_check);
void decide_lite_range(player_type *target_ptr, msa_type *msa_ptr);
bool decide_lite_projection(player_type *target_ptr, msa_type *msa_ptr);
void decide_lite_area(player_type *target_ptr, msa_type *msa_ptr);
