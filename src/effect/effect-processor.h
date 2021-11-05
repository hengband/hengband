﻿#pragma once

#include "system/angband.h"

//! project() の結果。
struct ProjectResult {
    bool notice{ false }; //!< プレイヤー/モンスター/アイテム/地形などに何らかの効果を及ぼしたか
    bool affected_player{ false }; //!< プレイヤーに何らかの効果を及ぼしたか(ラーニング判定用)

    ProjectResult() = default;
};

struct effect_PlayerType;
class PlayerType;
ProjectResult project(
    PlayerType *player_ptr, const MONSTER_IDX who, POSITION rad, POSITION y, POSITION x, const HIT_POINT dam, const EFFECT_ID typ,
    BIT_FLAGS flag);
