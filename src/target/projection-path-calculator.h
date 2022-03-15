#pragma once

#include "system/angband.h"
#include <utility>
#include <vector>

class PlayerType;
class projection_path {
public:
    projection_path(PlayerType *player_ptr, int range, int y1, int x1, int y2, int x2, BIT_FLAGS flg);
    std::vector<std::pair<int, int>>::const_iterator begin() const;
    std::vector<std::pair<int, int>>::const_iterator end() const;
    const std::pair<int, int> &front() const;
    const std::pair<int, int> &back() const;
    const std::pair<int, int> &operator[](int num) const;
    int path_num() const;

private:
    std::vector<std::pair<int, int>> position;
};
bool projectable(PlayerType *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2);
int get_max_range(PlayerType *player_ptr);
POSITION get_grid_y(uint16_t grid);
POSITION get_grid_x(uint16_t grid);
