#pragma once

#include "system/angband.h"

#include "system/player-type-definition.h"
void ang_sort(player_type *player_ptr, vptr u, vptr v, int n, bool (*ang_sort_comp)(player_type *, vptr, vptr, int, int),
    void (*ang_sort_swap)(player_type *, vptr, vptr, int, int));

bool ang_sort_comp_distance(player_type *player_ptr, vptr u, vptr v, int a, int b);
bool ang_sort_comp_importance(player_type *player_ptr, vptr u, vptr v, int a, int b);
void ang_sort_swap_position(player_type *player_ptr, vptr u, vptr v, int a, int b);

bool ang_sort_art_comp(player_type *player_ptr, vptr u, vptr v, int a, int b);
void ang_sort_art_swap(player_type *player_ptr, vptr u, vptr v, int a, int b);

bool ang_sort_comp_quest_num(player_type *player_ptr, vptr u, vptr v, int a, int b);
void ang_sort_swap_quest_num(player_type *player_ptr, vptr u, vptr v, int a, int b);

bool ang_sort_comp_pet(player_type *player_ptr, vptr u, vptr v, int a, int b);

bool ang_sort_comp_hook(player_type *player_ptr, vptr u, vptr v, int a, int b);
void ang_sort_swap_hook(player_type *player_ptr, vptr u, vptr v, int a, int b);

bool ang_sort_comp_monster_level(player_type *player_ptr, vptr u, vptr v, int a, int b);
bool ang_sort_comp_pet_dismiss(player_type *player_ptr, vptr u, vptr v, int a, int b);

bool ang_sort_comp_cave_temp(player_type *player_ptr, vptr u, vptr v, int a, int b);
void ang_sort_swap_cave_temp(player_type *player_ptr, vptr u, vptr v, int a, int b);
