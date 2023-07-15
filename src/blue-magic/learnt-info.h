#pragma once
/*!
 * @file learnt-info.h
 * @brief 青魔法の情報表示処理ヘッダ
 */

#include "system/angband.h"
#include <string>

enum class MonsterAbilityType;

class PlayerType;
PLAYER_LEVEL get_pseudo_monstetr_level(PlayerType *player_ptr);
std::string learnt_info(PlayerType *player_ptr, MonsterAbilityType power);
