#pragma once
/*!
 * @file weapon-shield.h
 * @brief 手装備持ち替え処理ヘッダ
 */

#include "system/angband.h"

class PlayerType;
void verify_equip_slot(PlayerType *player_ptr, INVENTORY_IDX i_idx);
