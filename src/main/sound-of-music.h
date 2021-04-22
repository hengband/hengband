#pragma once
/*!
 * @file sound-of-music.h
 * @brief BGM及び効果音のterm出力処理ヘッダ
 */

#include "system/angband.h"

#include <vector>

extern bool has_monster_music;

void bell(void);
void sound(int num);
errr play_music(int type, int num);
void select_floor_music(player_type *player_ptr);
void select_monster_music(player_type *player_ptr, const std::vector<MONSTER_IDX> &monster_list);
