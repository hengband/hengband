#pragma once

#include "system/angband.h"
#include <utility>
#include <vector>

// @todo pairをPos2Dとして再定義する.
class PlayerType;
class projection_path {
public:
    using const_iterator = std::vector<std::pair<int, int>>::const_iterator;

    projection_path(PlayerType *player_ptr, POSITION range, POSITION y1, POSITION x1, POSITION y2, POSITION x2, BIT_FLAGS flag);
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
POSITION get_grid_y(uint16_t grid);
POSITION get_grid_x(uint16_t grid);
