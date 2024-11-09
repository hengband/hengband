#pragma once

#include "system/angband.h"

enum class PathChecker {
    PROJECTION,
    LOS,
};

class MonsterEntity;
struct msa_type;
class PlayerType;
enum class TerrainCharacteristics;
bool adjacent_grid_check(PlayerType *player_ptr, MonsterEntity *m_ptr, POSITION *yp, POSITION *xp, TerrainCharacteristics f_flag, PathChecker checker);
void decide_lite_range(PlayerType *player_ptr, msa_type *msa_ptr);
bool decide_lite_projection(PlayerType *player_ptr, msa_type *msa_ptr);
void decide_lite_area(PlayerType *player_ptr, msa_type *msa_ptr);
