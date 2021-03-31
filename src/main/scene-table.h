#pragma once

#include <vector>

enum class scene_table_type {
    SCENE_TABLE_FLOOR = 1,
    SCENE_TABLE_MONSTER = 2,
};

struct scene_type {
    int type = 0; //!< シチュエーションカテゴリ
    int val = 0; //!< シチュエーション項目
};

using scene_type_list = std::vector<scene_type>;
extern scene_type_list& get_scene_type_list(scene_table_type type);
