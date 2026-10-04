/*!
 * @brief TimedEffectクラステンプレートのテスト
 *
 * 時限効果の共通部分 (値の保持、有効判定、負値ガード) を検証する。
 * 個別の効果に固有のロジックは test-player-cut.cpp / test-player-stun.cpp で扱う。
 */

#include "timed-effect/timed-effect.h"

#include "timed-effect/timed-effects.h"

#include <doctest/doctest.h>

#include <limits>
#include <stdexcept>
#include <type_traits>

namespace {

using TestEffect = TimedEffect<struct TagTestEffect>;

//! 値の保持を確かめる代表値 (0、最小の有効値、実用域、ゲーム上の上限、short の上限)
constexpr short TEST_VALUES[] = { 0, 1, 100, 10000, std::numeric_limits<short>::max() };

}

// タグ型が異なる効果同士が別の型になっていることを確認する。
// 同一の型になっていると、盲目と混乱を取り違えてもコンパイルが通ってしまう。
static_assert(!std::is_same_v<PlayerBlindness, PlayerConfusion>);

TEST_CASE("TimedEffect is inactive just after construction")
{
    const TestEffect effect;

    CHECK(effect.current() == 0);
    CHECK_FALSE(effect.is_active());
}

TEST_CASE("TimedEffect holds the value set and is active only when it is positive")
{
    TestEffect effect;

    for (const auto value : TEST_VALUES) {
        CAPTURE(value);
        effect.set(value);
        CHECK(effect.current() == value);
        CHECK(effect.is_active() == (value > 0));
    }
}

TEST_CASE("TimedEffect is cleared by reset")
{
    TestEffect effect;
    effect.set(100);
    REQUIRE(effect.is_active());

    effect.reset();

    CHECK(effect.current() == 0);
    CHECK_FALSE(effect.is_active());
}

TEST_CASE("TimedEffect rejects a negative value and keeps the previous one")
{
    TestEffect effect;
    effect.set(50);

    CHECK_THROWS_AS(effect.set(-1), std::invalid_argument);
    CHECK_THROWS_AS(effect.set(std::numeric_limits<short>::min()), std::invalid_argument);

    CHECK(effect.current() == 50);
}
