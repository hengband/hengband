#pragma once
/*!
 * @file fixed-art-generator.h
 * @brief 固定アーティファクトの生成処理ヘッダ
 */

#include "system/angband.h"

enum class FixedArtifactId : short;
class ItemEntity;
class PlayerType;
bool create_named_art(PlayerType *player_ptr, FixedArtifactId a_idx, POSITION y, POSITION x);
void apply_artifact(PlayerType *player_ptr, ItemEntity *o_ptr);
