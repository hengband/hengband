#pragma once
/*!
 * @file blue-magic-caster.h
 * @brief 青魔法のその他系統の呪文定義と詠唱時分岐処理ヘッダ
 */

#include "system/angband.h"

enum class RF_ABILITY;

bool cast_learned_spell(player_type *caster_ptr, RF_ABILITY spell, const bool success);
