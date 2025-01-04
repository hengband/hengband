#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include "util/point-2d.h"
#include <span>
#include <vector>

class FloorType;
class PlayerType;
class ProjectionPath;
bool in_disintegration_range(FloorType *floor_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
void breath_shape(PlayerType *player_ptr, const ProjectionPath &path, int dist, int *pgrids, std::span<Pos2D> positions, std::span<int> gm, int *pgm_rad, int rad, const Pos2D &pos_source, const Pos2D &pos_target, AttributeType typ);
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
