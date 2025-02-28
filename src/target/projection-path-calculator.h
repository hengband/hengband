#pragma once

#include "util/point-2d.h"
#include <cstdint>
#include <vector>

class FloorType;
class PlayerType;
class ProjectionPath {
public:
    using pp_const_iterator = std::vector<Pos2D>::const_iterator;

    ProjectionPath(const FloorType &floor, int range, const Pos2D &p_pos, const Pos2D &pos_src, const Pos2D &pos_dst, uint32_t flag);
    pp_const_iterator begin() const;
    pp_const_iterator end() const;
    const Pos2D &front() const;
    const Pos2D &back() const;
    const Pos2D &operator[](int num) const;
    int path_num() const;

private:
    std::vector<Pos2D> positions;
};

bool projectable(PlayerType *player_ptr, const Pos2D &pos1, const Pos2D &pos2);
