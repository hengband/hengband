#pragma once

#include "util/point-2d.h"

enum class TerrainTag;
class PlayerType;
void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, TerrainTag tag);
void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, short terrain_id);
