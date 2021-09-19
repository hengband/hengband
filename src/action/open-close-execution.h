#pragma once
/*!
 * @file open-close-execution.h
 * @brief 扉や箱を開ける処理のヘッダ
 */

#include "system/angband.h"

struct player_type;
bool exe_open(player_type *player_ptr, POSITION y, POSITION x);
bool exe_close(player_type *player_ptr, POSITION y, POSITION x);
bool easy_open_door(player_type *player_ptr, POSITION y, POSITION x);
bool exe_disarm(player_type *player_ptr, POSITION y, POSITION x, DIRECTION dir);
bool exe_disarm_chest(player_type *player_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx);
bool exe_bash(player_type *player_ptr, POSITION y, POSITION x, DIRECTION dir);
