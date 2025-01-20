#pragma once

#include "system/angband.h"
#include "util/point-2d.h"
#include <string>
#include <vector>

/*** Terrain feature variables ***/
extern FEAT_IDX feat_trap_open;
extern FEAT_IDX feat_trap_armageddon;
extern FEAT_IDX feat_trap_piranha;
extern FEAT_IDX feat_pattern_start;
extern FEAT_IDX feat_pattern_1;
extern FEAT_IDX feat_pattern_2;
extern FEAT_IDX feat_pattern_3;
extern FEAT_IDX feat_pattern_4;
extern FEAT_IDX feat_pattern_end;
extern FEAT_IDX feat_pattern_old;
extern FEAT_IDX feat_pattern_exit;
extern FEAT_IDX feat_pattern_corrupted;
extern FEAT_IDX feat_undetected;

extern FEAT_IDX feat_wall_outer;
extern FEAT_IDX feat_wall_inner;
extern FEAT_IDX feat_wall_solid;
extern FEAT_IDX feat_ground_type[100];
extern FEAT_IDX feat_wall_type[100];

enum class TerrainTag;
class FloorType;
class PlayerType;
FEAT_IDX feat_locked_door_random(int door_type);
FEAT_IDX feat_jammed_door_random(int door_type);
void cave_set_feat(PlayerType *player_ptr, const Pos2D &pos, TerrainTag tag);
void cave_set_feat(PlayerType *player_ptr, POSITION y, POSITION x, FEAT_IDX feat);
