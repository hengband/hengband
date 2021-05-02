#pragma once
/*!
 * @file mutation-execution.h
 * @brief プレイヤーの変異能力実行ヘッダ
 */

enum class MUTA;
typedef struct player_type player_type;
bool exe_mutation_power(player_type *creature_ptr, MUTA power);
