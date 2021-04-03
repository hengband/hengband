#pragma once

#include "main/scene-table.h"
#include "system/angband.h"

int get_scene_floor_count();
void refresh_scene_floor(player_type *player_ptr, scene_type_list &list, int start_index);
