#pragma once
/*!
 * @file fixed-art-generator.h
 * @brief 固定アーティファクトの生成処理ヘッダ
 */

#include "system/angband.h"

enum class FixedArtifactId : short;
class ArtifactType;
class ObjectType;
class PlayerType;
bool create_named_art(PlayerType *player_ptr, FixedArtifactId a_idx, POSITION y, POSITION x);
bool make_artifact(PlayerType *player_ptr, ObjectType *o_ptr);
void apply_artifact(PlayerType *player_ptr, ObjectType *o_ptr);
bool make_artifact_special(PlayerType *player_ptr, ObjectType *o_ptr);
