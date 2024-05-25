#pragma once

#include "system/angband.h"

#include "system/player-type-definition.h"
void ang_sort(PlayerType *player_ptr, vptr u, vptr v, int n, bool (*ang_sort_comp)(PlayerType *, vptr, vptr, int, int),
    void (*ang_sort_swap)(PlayerType *, vptr, vptr, int, int));

bool ang_sort_comp_distance(PlayerType *player_ptr, vptr u, vptr v, int a, int b);
bool ang_sort_comp_importance(PlayerType *player_ptr, vptr u, vptr v, int a, int b);
void ang_sort_swap_position(PlayerType *player_ptr, vptr u, vptr v, int a, int b);
