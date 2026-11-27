/*!
 * @file magic-device-chance.h
 * @brief 魔道具 (発動・ロッド・魔法棒・杖) を使うときの成否判定
 */

#pragma once

#include "system/h-type.h"

class PlayerType;

/*!
 * @brief 発動・ロッドの抽選に使う成功と失敗の重み
 */
struct RodActivationWeights {
    int success;
    int failure;
};

RodActivationWeights calc_rod_activation_weights(ACTION_SKILL_POWER skill, int level);
bool try_zap_rod_or_activate(PlayerType *player_ptr, int level);
int calc_wand_staff_level(int baseitem_level);
bool try_use_wand_or_staff(PlayerType *player_ptr, int level);
