#pragma once

#include "system/angband.h"

#include <iterator>

struct scene_type;
// シチュエーション判定関数。valueに設定した場合trueを返す。
typedef bool (*SCENE_DEF_FUNC)(player_type *player_ptr, scene_type *value);

struct scene_type {
    int type; //!< シチュエーションカテゴリ
    int val; //!< シチュエーション項目

    bool update(player_type *player_ptr, scene_type *value)
    {
        return scene_def(player_ptr, value);
    }

    scene_type(SCENE_DEF_FUNC f)
        : scene_def(f)
    {
    }

private:
    SCENE_DEF_FUNC scene_def; //!< シチュエーション判定関数
};

void refresh_scene_table(player_type *player_ptr);
std::vector<scene_type>::iterator get_scene_table_iterator();
