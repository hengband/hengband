#pragma once
/*!
 * @file random-art-generator.h
 * @brief ランダムアーティファクトの生成メインヘッダ / Artifact code
 */

class ObjectType;
class PlayerType;
bool become_random_artifact(PlayerType *player_ptr, ObjectType *o_ptr, bool a_scroll);
