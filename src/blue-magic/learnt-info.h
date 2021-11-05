#pragma once
/*!
 * @file learnt-info.h
 * @brief 青魔法の情報表示処理ヘッダ
 */

#include "system/angband.h"

enum class RF_ABILITY;

class PlayerType;
PLAYER_LEVEL get_pseudo_monstetr_level(PlayerType *player_ptr);
void learnt_info(PlayerType *player_ptr, char *p, RF_ABILITY power);
