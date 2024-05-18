#pragma once

#include "util/point-2d.h"
#include <cstdint>
#include <utility>
#include <vector>

// @todo pairをPos2Dとして再定義する.
class PlayerType;
class ProjectionPath {
public:
    using const_iterator = std::vector<std::pair<int, int>>::const_iterator;

    ProjectionPath(PlayerType *player_ptr, int range, const Pos2D &pos_src, const Pos2D &pos_dst, uint32_t flag);
    const_iterator begin() const;
    const_iterator end() const;
    const std::pair<int, int> &front() const;
    const std::pair<int, int> &back() const;
    const std::pair<int, int> &operator[](int num) const;
    int path_num() const;

private:
    std::vector<std::pair<int, int>> position;
};

bool projectable(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
