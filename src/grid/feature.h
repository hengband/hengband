#pragma once

#include "util/point-2d.h"

/*** Terrain feature variables ***/
extern short feat_wall_outer;
extern short feat_wall_inner;
extern short feat_wall_solid;
extern short feat_ground_type[100];
extern short feat_wall_type[100];

enum class TerrainTag;
class PlayerType;
void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, TerrainTag tag);
void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, short terrain_id);
