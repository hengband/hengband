#pragma once
/*!
 * @file blue-magic-caster.h
 * @brief 青魔法のその他系統の呪文定義と詠唱時分岐処理ヘッダ
 */

enum class RF_ABILITY;

class PlayerType;
bool cast_learned_spell(PlayerType *player_ptr, RF_ABILITY spell, const bool success);
