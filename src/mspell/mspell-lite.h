#pragma once

#include "system/angband.h"

class MonsterEntity;
struct msa_type;
class PlayerType;
typedef bool (*path_check_pf)(PlayerType *, POSITION, POSITION, POSITION, POSITION);
enum class TerrainCharacteristics;
bool adjacent_grid_check(PlayerType *player_ptr, MonsterEntity *m_ptr, POSITION *yp, POSITION *xp, TerrainCharacteristics f_flag, path_check_pf path_check);
void decide_lite_range(PlayerType *player_ptr, msa_type *msa_ptr);
bool decide_lite_projection(PlayerType *player_ptr, msa_type *msa_ptr);
void decide_lite_area(PlayerType *player_ptr, msa_type *msa_ptr);
