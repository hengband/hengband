#pragma once
/*!
 * @file blue-magic-caster.h
 * @brief 青魔法のその他系統の呪文定義と詠唱時分岐処理ヘッダ
 */

enum class MonsterAbilityType;

class PlayerType;
bool cast_learned_spell(PlayerType *player_ptr, MonsterAbilityType spell, const bool success);
