#pragma once
/*!
 * @file activation-execution.h
 * @brief アイテムの発動実行ヘッダ
 */

#include "system/angband.h"

typedef struct player_type player_type;
void exe_activate(player_type *user_ptr, INVENTORY_IDX item);
