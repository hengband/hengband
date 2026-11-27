/*!
 * @file magic-device-chance.cpp
 * @brief 魔道具 (発動・ロッド・魔法棒・杖) を使うときの成否判定の実装
 */

#include "object-use/magic-device-chance.h"
#include "player-base/player-class.h"
#include "system/gamevalue.h"
#include "system/player-type-definition.h"
#include "term/z-rand.h"
#include "timed-effect/timed-effects.h"
#include <algorithm>

/*!
 * @brief 魔道具を使うときの技能を返す
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 魔道具技能。混乱していれば半分にする
 */
static ACTION_SKILL_POWER calc_device_skill(PlayerType *player_ptr)
{
    auto skill = player_ptr->skill_dev;
    if (player_ptr->effects()->confusion().is_active()) {
        skill /= 2;
    }

    return skill;
}

/*!
 * @brief 発動・ロッドの基本難易度と技能から抽選用の成功・失敗の重みを求める
 * @param skill 混乱などの補正後の魔道具技能 (符号付き 16 ビット)
 * @param level 発動・ロッドのレベル
 * @return 成功と失敗の重み (どちらも USE_DEVICE 以上)
 * @details level は各定義の reader で 0〜127 に制限される。この範囲と技能の型なら、重みと抽選のときの 2 倍も int に収まる。
 */
RodActivationWeights calc_rod_activation_weights(ACTION_SKILL_POWER skill, int level)
{
    auto success_weight = int{ skill };
    auto failure_weight = level + 5;
    if (success_weight > failure_weight) {
        failure_weight -= (success_weight - failure_weight) * 2;
    } else {
        success_weight -= (failure_weight - success_weight) * 2;
    }

    return { std::max(success_weight, USE_DEVICE), std::max(failure_weight, USE_DEVICE) };
}

/*!
 * @brief 発動・ロッドを使えるかを抽選する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param level 発動・ロッドのレベル
 * @return 使えたら true
 * @details 重みの大きい方の 2 倍の範囲で抽選する。バーサーカーは乱数を引かずに必ず失敗する。
 */
bool try_zap_rod_or_activate(PlayerType *player_ptr, int level)
{
    if (PlayerClass(player_ptr).equals(PlayerClassType::BERSERKER)) {
        return false;
    }

    const auto weights = calc_rod_activation_weights(calc_device_skill(player_ptr), level);

    if (weights.success > weights.failure) {
        return randint0(weights.success * 2) >= weights.failure;
    }

    return randint0(weights.failure * 2) < weights.success;
}
