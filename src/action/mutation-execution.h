#pragma once
/*!
 * @file mutation-execution.h
 * @brief プレイヤーの変異能力実行ヘッダ
 */

enum class MUTA;
class player_type;
bool exe_mutation_power(player_type *player_ptr, MUTA power);
