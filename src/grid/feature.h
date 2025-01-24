#pragma once

#include "system/angband.h"
#include "util/point-2d.h"

/*** Terrain feature variables ***/
extern FEAT_IDX feat_wall_outer;
extern FEAT_IDX feat_wall_inner;
extern FEAT_IDX feat_wall_solid;
extern FEAT_IDX feat_ground_type[100];
extern FEAT_IDX feat_wall_type[100];

enum class DoorKind;
enum class TerrainTag;
class FloorType;
class PlayerType;
void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, TerrainTag tag);
void cave_set_feat(PlayerType *player_ptr, POSITION y, POSITION x, FEAT_IDX feat);
