﻿#pragma once
/*!
 * @file weapon-shield.h
 * @brief 手装備持ち替え処理ヘッダ
 */

#include "system/angband.h"

class player_type;
void verify_equip_slot(player_type *player_ptr, INVENTORY_IDX item);
