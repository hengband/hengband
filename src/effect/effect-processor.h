#pragma once

#include "effect/attribute-types.h"
#include "system/angband.h"

//! project() の結果。
struct ProjectResult {
    bool notice{ false }; //!< プレイヤー/モンスター/アイテム/地形などに何らかの効果を及ぼしたか
    bool affected_player{ false }; //!< プレイヤーに何らかの効果を及ぼしたか(ラーニング判定用)

    ProjectResult() = default;
};

class CapturedMonsterType;
class EffectPlayerType;
class PlayerType;
ProjectResult project(
    PlayerType *player_ptr, const MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, const int dam, const AttributeType typ,
    BIT_FLAGS flag, std::optional<CapturedMonsterType *> cap_mon_ptr = std::nullopt);
