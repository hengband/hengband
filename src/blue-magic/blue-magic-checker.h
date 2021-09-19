#pragma once
/*!
 * @file blue-magic-checker.h
 * @brief 青魔法の処理ヘッダ / Blue magic
 */

#include "mind/mind-blue-mage.h"
#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

enum blue_magic_type : int;

struct player_type;
void learn_spell(player_type *player_ptr, int monspell);
void set_rf_masks(EnumClassFlagGroup<RF_ABILITY> &ability_flags, blue_magic_type mode);
