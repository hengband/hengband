#pragma once

#include "system/angband.h"

enum class PathChecker;
enum class TerrainCharacteristics;
struct msa_type;
class MonsterEntity;
class PlayerType;
bool adjacent_grid_check(PlayerType *player_ptr, const MonsterEntity &monster, POSITION *yp, POSITION *xp, TerrainCharacteristics f_flag, PathChecker checker);
void decide_lite_range(PlayerType *player_ptr, msa_type *msa_ptr);
bool decide_lite_projection(PlayerType *player_ptr, msa_type *msa_ptr);
void decide_lite_area(PlayerType *player_ptr, msa_type *msa_ptr);
