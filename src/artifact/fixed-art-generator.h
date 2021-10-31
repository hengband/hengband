#pragma once
/*!
 * @file fixed-art-generator.h
 * @brief 固定アーティファクトの生成処理ヘッダ
 */

#include "system/angband.h"

typedef struct artifact_type artifact_type;
struct object_type;
struct player_type;
bool create_named_art(player_type *player_ptr, ARTIFACT_IDX a_idx, POSITION y, POSITION x);
bool make_artifact(player_type *player_ptr, object_type *o_ptr);
artifact_type *apply_artifact(player_type *player_ptr, object_type *o_ptr);
bool make_artifact_special(player_type *player_ptr, object_type *o_ptr);
