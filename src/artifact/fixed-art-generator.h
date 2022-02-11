#pragma once
/*!
 * @file fixed-art-generator.h
 * @brief 固定アーティファクトの生成処理ヘッダ
 */

#include "system/angband.h"

typedef struct artifact_type artifact_type;
class ObjectType;
class PlayerType;
bool create_named_art(PlayerType *player_ptr, ARTIFACT_IDX a_idx, POSITION y, POSITION x);
bool make_artifact(PlayerType *player_ptr, ObjectType *o_ptr);
artifact_type *apply_artifact(PlayerType *player_ptr, ObjectType *o_ptr);
bool make_artifact_special(PlayerType *player_ptr, ObjectType *o_ptr);
