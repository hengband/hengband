#pragma once
/*!
 * @file scene-table-floor.h
 * @brief フロアの状況に応じたBGM設定処理ヘッダ
 */

#include "main/scene-table.h"
#include "system/angband.h"

int get_scene_floor_count();
void refresh_scene_floor(PlayerType *player_ptr, scene_type_list &list, int start_index);
