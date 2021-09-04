#pragma once
/*!
 * @file learnt-info.h
 * @brief 青魔法の情報表示処理ヘッダ
 */

#include "system/angband.h"

enum class RF_ABILITY;

struct player_type;
PLAYER_LEVEL get_pseudo_monstetr_level(player_type *caster_ptr);
void learnt_info(player_type *learner_ptr, char *p, RF_ABILITY power);
