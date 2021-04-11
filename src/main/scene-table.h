#pragma once

#include "player/player-status.h"
#include "system/angband.h"
#include "system/monster-race-definition.h"

#include <vector>

struct scene_type {
    int type = 0; //!< シチュエーションカテゴリ
    int val = 0; //!< シチュエーション項目
};

using scene_type_list = std::vector<scene_type>;

void interrupt_scene(int type, int val);
void refresh_scene_table(player_type *player_ptr);
void refresh_scene_table(player_type *player_ptr, const std::vector<MONSTER_IDX> &monster_list);
scene_type_list &get_scene_type_list(int val);
