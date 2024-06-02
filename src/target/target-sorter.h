#pragma once

#include "util/point-2d.h"
#include <vector>

class FloorType;
class TargetSorter {
public:
    TargetSorter(const Pos2D &p_pos);
    bool compare_importance(const FloorType &floor, const Pos2D &pos_a, const Pos2D &pos_b) const;
    bool compare_distance(const Pos2D &pos_a, const Pos2D &pos_b) const;

private:
    Pos2D p_pos; //!< プレイヤーの現在位置

    int calc_double_distance(const Pos2D &pos_a) const;
};
