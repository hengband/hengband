#pragma once

#include "system/angband.h"

#include <iterator>
#include <vector>

struct scene_type;
// シチュエーション判定関数。valueに設定した場合trueを返す。
using scene_def_func = bool (*)(player_type *player_ptr, scene_type *value);

struct scene_type {
    int type = 0; //!< シチュエーションカテゴリ
    int val = 0; //!< シチュエーション項目

    bool update(player_type *player_ptr, scene_type *value)
    {
        return scene_def(player_ptr, value);
    }

    scene_type(scene_def_func f)
        : scene_def(f)
    {
    }

private:
    scene_def_func scene_def; //!< シチュエーション判定関数
};

void refresh_scene_table(player_type *player_ptr);
using scene_iterator = std::vector<scene_type>::const_iterator;
scene_iterator get_scene_table_iterator();
