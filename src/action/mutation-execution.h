#pragma once
/*!
 * @file mutation-execution.h
 * @brief プレイヤーの変異能力実行ヘッダ
 */

#include "system/angband.h"

enum class MUTA;
bool exe_mutation_power(player_type *creature_ptr, MUTA power);
