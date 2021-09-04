#pragma once
/*!
 * @file game-data-initializer.h
 * @brief 変愚蛮怒のゲームデータ初期化ヘッダファイル
 */

#include "system/angband.h"

struct player_type;
errr init_quests(void);
errr init_other(player_type *player_ptr);
errr init_object_alloc(void);
errr init_alloc(void);
