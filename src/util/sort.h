#pragma once

#include <vector>

class PlayerType;
void ang_sort(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int n, bool (*ang_sort_comp)(PlayerType *, std::vector<int> &, std::vector<int> &, int, int));
bool ang_sort_comp_distance(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int a, int b);
bool ang_sort_comp_importance(PlayerType *player_ptr, std::vector<int> &ys, std::vector<int> &xs, int a, int b);
