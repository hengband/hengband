#pragma once
/*!
 * @file random-art-characteristics.h
 * @brief ランダムアーティファクトのバイアス付加処理ヘッダ
 */

struct object_type;;
struct player_type;
void curse_artifact(player_type *player_ptr, object_type *o_ptr);
void get_random_name(object_type *o_ptr, char *return_name, bool armour, int power);
bool has_extreme_damage_rate(player_type *player_ptr, object_type *o_ptr);
