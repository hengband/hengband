#pragma once

#include "system/h-type.h"
#include "util/point-2d.h"

class PlayerType;
class TerrainType;
class Grid;
enum class AttributeType;
class AbstractAttribute {
public:
    virtual ~AbstractAttribute() = default;
    virtual bool affect_feature(PlayerType *const player_ptr, const MONSTER_IDX src_idx, const TerrainType &terrain, Grid &grid, const Pos2D &pos, const int dam, const bool known) = 0;
};
