#pragma once

#include "system/angband-system.h"
#include "term/z-rand.h"
#include "util/finalizer.h"

#include <cstdint>

namespace test {

//! テストで既定として使う乱数シード
constexpr uint32_t DEFAULT_RNG_SEED = 12345;

/*!
 * @brief ゲームの乱数生成器をテスト用のシードで初期化する
 * @param seed 設定するシード値
 * @return スコープを抜けるときに乱数生成器の状態を元へ戻すファイナライザ
 * @details 戻り値は必ず変数で受けること。受けないとその場で復元されてしまう。
 *          受け損ねた場合は [[nodiscard]] により警告が出る
 *          (警告をエラーとして扱う CI や Visual Studio のビルドでは失敗する)。
 */
[[nodiscard]] inline auto scoped_rng(uint32_t seed = DEFAULT_RNG_SEED)
{
    auto rng_backup = AngbandSystem::get_instance().get_rng();
    Rand_state_init(seed);
    return util::make_finalizer([rng_backup] { AngbandSystem::get_instance().set_rng(rng_backup); });
}

}
