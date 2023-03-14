#pragma once
/*!
 * @file scene-table.h
 * @brief BGM選曲の基本処理部分ヘッダ
 */

#include <vector>

#include "system/angband.h"

struct scene_type {
    TERM_XTRA type = TERM_XTRA::NONE; //!< シチュエーションカテゴリ
    int val = 0; //!< シチュエーション項目
};

using scene_type_list = std::vector<scene_type>;

class PlayerType;
void interrupt_scene(TERM_XTRA type, int val);
void refresh_scene_table(PlayerType *player_ptr);
void refresh_scene_table(PlayerType *player_ptr, const std::vector<MONSTER_IDX> &monster_list);
scene_type_list &get_scene_type_list(int val);
