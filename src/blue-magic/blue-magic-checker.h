#pragma once
/*!
 * @file blue-magic-checker.h
 * @brief 青魔法の処理ヘッダ / Blue magic
 */

#include "system/angband.h"

enum blue_magic_type : int;

void learn_spell(player_type *learner_ptr, int monspell);
void set_rf_masks(FlagGroup<RF_ABILITY>& ability_flags, blue_magic_type mode);
