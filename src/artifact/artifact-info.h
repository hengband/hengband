#pragma once
/*!
 * @file artifact-info.h
 * @brief アーティファクトの発動効果取得関数ヘッダ
 */

typedef struct activation_type activation_type;
typedef struct object_type object_type;
typedef struct player_type player_type;
int activation_index(const object_type *o_ptr);
const activation_type *find_activation_info(const object_type *o_ptr);
