#pragma once

#include <vector>

enum class SortKind {
    DISTANCE,
    IMPORTANCE,
};

class PlayerType;
void ang_sort(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, SortKind kind);
bool ang_sort_comp_distance(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int a, int b);
bool ang_sort_comp_importance(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int a, int b);
