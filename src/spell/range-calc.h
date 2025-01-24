#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"
#include "util/point-2d.h"
#include <span>
#include <utility>
#include <vector>

class FloorType;
class PlayerType;
class ProjectionPath;
bool in_disintegration_range(const FloorType &floor, const Pos2D &pos1, const Pos2D &pos2);
std::vector<std::pair<int, Pos2D>> breath_shape(PlayerType *player_ptr, const ProjectionPath &path, int dist, int rad, const Pos2D &pos_source, const Pos2D &pos_target, AttributeType typ);
std::vector<std::pair<int, Pos2D>> ball_shape(PlayerType *player_ptr, const Pos2D &center, int rad, AttributeType typ);
POSITION dist_to_line(POSITION y, POSITION x, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
