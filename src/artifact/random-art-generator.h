#pragma once
/*!
 * @file random-art-generator.h
 * @brief ランダムアーティファクトの生成メインヘッダ / Artifact code
 */

class ItemEntity;
class PlayerType;
bool become_random_artifact(PlayerType *player_ptr, ItemEntity *o_ptr, bool a_scroll);
