﻿#pragma once
/*!
 * @file blue-magic-checker.h
 * @brief 青魔法の処理ヘッダ / Blue magic
 */

#include "mind/mind-blue-mage.h"
#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"

enum class BlueMagicType;

class PlayerType;
void learn_spell(PlayerType *player_ptr, RF_ABILITY monspell);
void set_rf_masks(EnumClassFlagGroup<RF_ABILITY> &ability_flags, BlueMagicType type);
