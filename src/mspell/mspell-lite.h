#pragma once

#include "util/point-2d.h"
#include <optional>

enum class PathChecker;
enum class TerrainCharacteristics;
struct msa_type;
class MonsterEntity;
class PlayerType;
std::optional<Pos2D> adjacent_grid_check(PlayerType *player_ptr, const MonsterEntity &monster, const Pos2D &pos_initial, TerrainCharacteristics tc, PathChecker checker);
void decide_lite_range(PlayerType *player_ptr, msa_type *msa_ptr);
bool decide_lite_projection(PlayerType *player_ptr, msa_type *msa_ptr);
void decide_lite_area(PlayerType *player_ptr, msa_type *msa_ptr);
