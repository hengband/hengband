#pragma once
/*!
 * @file scene-table-monster.h
 * @brief モンスターの遭遇状況に応じたBGM設定処理ヘッダ
 */

#include "main/scene-table.h"
#include "system/angband.h"

class PlayerType;
void clear_scene_target_monster();
void set_temp_mute_scene_monster(int sec);
int get_scene_monster_count();
void refresh_scene_monster(PlayerType *player_ptr, const std::vector<MONSTER_IDX> &monster_list, scene_type_list &list, int from_index);
