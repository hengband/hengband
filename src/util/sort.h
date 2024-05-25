#pragma once

#include <vector>

enum class SortKind {
    DISTANCE,
    IMPORTANCE,
};

class PlayerType;
void ang_sort(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, SortKind kind);
