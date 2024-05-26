#pragma once

#include "util/point-2d.h"
#include <vector>

enum class SortKind {
    DISTANCE,
    IMPORTANCE,
};

class FloorType;
class TargetSorter {
public:
    TargetSorter() = default;
    void sort(const FloorType &floor, const Pos2D &p_pos, std::vector<int> &ys, std::vector<int> &xs, SortKind kind);
};
