#pragma once
/*!
 * @file random-art-generator.h
 * @brief ランダムアーティファクトの生成メインヘッダ / Artifact code
 */

struct object_type;
class PlayerType;
bool become_random_artifact(PlayerType *player_ptr, object_type *o_ptr, bool a_scroll);
