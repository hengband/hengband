#pragma once
/*!
 * @file activation-execution.h
 * @brief アイテムの発動実行ヘッダ
 */

#include "system/angband.h"

class player_type;
void exe_activate(player_type *player_ptr, INVENTORY_IDX item);
