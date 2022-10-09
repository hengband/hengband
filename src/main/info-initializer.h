#pragma once
/*!
 * @file info-initializer.h
 * @brief 変愚蛮怒のゲームデータ解析処理ヘッダ
 */

#include "system/angband.h"

class PlayerType;
errr init_artifacts_info();
errr init_baseitems_info();
errr init_class_magics_info();
errr init_dungeons_info();
errr init_egos_info();
errr init_terrains_info();
errr init_misc(PlayerType *player_ptr);
errr init_r_info();
errr init_v_info();
errr init_s_info();
