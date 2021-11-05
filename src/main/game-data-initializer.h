﻿#pragma once
/*!
 * @file game-data-initializer.h
 * @brief 変愚蛮怒のゲームデータ初期化ヘッダファイル
 */

#include "system/angband.h"

class PlayerType;
void init_quests(void);
void init_other(PlayerType *player_ptr);
void init_alloc(void);
