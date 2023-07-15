#pragma once
/*!
 * @file mutation-execution.h
 * @brief プレイヤーの変異能力実行ヘッダ
 */

enum class PlayerMutationType;
class PlayerType;
bool exe_mutation_power(PlayerType *player_ptr, PlayerMutationType power);
