#pragma once
#include "effect/attribute/abstract-attribute.h"

class FireAttribute : public AbstractAttribute {
public:
    bool affect_feature(PlayerType *const player_ptr, const MONSTER_IDX src_idx, const TerrainType &terrain, Grid &grid, const Pos2D &pos, const int dam, const bool known) override;
};
