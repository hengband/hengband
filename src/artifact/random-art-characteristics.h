#pragma once
/*!
 * @file random-art-characteristics.h
 * @brief ランダムアーティファクトのバイアス付加処理ヘッダ
 */

class ItemEntity;
class PlayerType;
void curse_artifact(PlayerType *player_ptr, ItemEntity *o_ptr);
void get_random_name(ItemEntity *o_ptr, char *return_name, bool armour, int power);
bool has_extreme_damage_rate(PlayerType *player_ptr, ItemEntity *o_ptr);
