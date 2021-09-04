#pragma once
/*!
 * @file artifact-info.h
 * @brief アーティファクトの発動効果取得関数ヘッダ
 */

struct activation_type;
struct object_type;;
struct player_type;
int activation_index(const object_type *o_ptr);
const activation_type *find_activation_info(const object_type *o_ptr);
