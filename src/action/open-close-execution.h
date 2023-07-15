#pragma once
/*!
 * @file open-close-execution.h
 * @brief 扉や箱を開ける処理のヘッダ
 */

#include "system/angband.h"

class PlayerType;
bool exe_open(PlayerType *player_ptr, POSITION y, POSITION x);
bool exe_close(PlayerType *player_ptr, POSITION y, POSITION x);
bool easy_open_door(PlayerType *player_ptr, POSITION y, POSITION x);
bool exe_disarm(PlayerType *player_ptr, POSITION y, POSITION x, DIRECTION dir);
bool exe_disarm_chest(PlayerType *player_ptr, POSITION y, POSITION x, OBJECT_IDX o_idx);
bool exe_bash(PlayerType *player_ptr, POSITION y, POSITION x, DIRECTION dir);
