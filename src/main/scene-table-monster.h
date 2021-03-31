#pragma once

#include "main/scene-table.h"
#include "system/angband.h"

extern void refresh_monster_scene(player_type *player_ptr, const std::vector<MONSTER_IDX> &monster_list);
extern scene_type_list &get_monster_scene_type_list();
