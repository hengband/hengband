/*!
 * @brief 得意さの段階から職業別の魔法の値を求める式 (spell/magic-aptitude.cpp) のテスト
 */

#include "spell/magic-aptitude.h"
#include "spell/magic-type.h"
#include <array>
#include <doctest/doctest.h>
#include <utility>

namespace {
//! 初期データの傾き (習得レベル 30、消費MP 15、難易度 3)
constexpr AptitudeSteps DEFAULT_STEPS{ 30, 15, 3 };

//! 消費MP を動かさない傾き (段階 ±9 では消費MP の倍率が 1% 未満になるので、習得レベルだけを確かめるときに使う)
constexpr AptitudeSteps LEVEL_ONLY_STEPS{ 30, 0, 3 };

magic_type make_base(int level, int mana, int difficulty, int exp)
{
    magic_type base;
    base.set_learn_level(static_cast<short>(level));
    base.smana = mana;
    base.sfail = difficulty;
    base.sexp = exp;
    return base;
}

int derive_level(int base_level, int aptitude, const AptitudeSteps &steps = DEFAULT_STEPS)
{
    const auto derived = derive_magic_by_aptitude(make_base(base_level, 10, 50, 1), steps, aptitude);
    REQUIRE(derived.has_value());
    REQUIRE(derived->is_learnable());
    return derived->get_learn_level();
}
}

TEST_CASE("derive_magic_by_aptitude keeps the base values at aptitude 0")
{
    const auto base = make_base(23, 17, 45, 8);
    const auto derived = derive_magic_by_aptitude(base, DEFAULT_STEPS, 0);
    REQUIRE(derived.has_value());
    CHECK(derived->get_learn_level() == 23);
    CHECK(derived->smana == 17);
    CHECK(derived->sfail == 45);
    CHECK(derived->sexp == 8);
}

TEST_CASE("derive_magic_by_aptitude moves the learn level most around level 25")
{
    // 仕様の早見表 (段階 -3〜+3)
    constexpr std::array<std::pair<int, std::array<int, 7>>, 6> player_level_table{ {
        { 5, { 9, 8, 6, 5, 4, 2, 1 } },
        { 10, { 17, 15, 12, 10, 8, 5, 3 } },
        { 20, { 31, 27, 24, 20, 16, 13, 9 } },
        { 30, { 41, 37, 34, 30, 26, 23, 19 } },
        { 40, { 47, 45, 42, 40, 38, 35, 33 } },
        { 45, { 49, 48, 46, 45, 44, 42, 41 } },
    } };

    for (const auto &[base_level, expected] : player_level_table) {
        for (auto aptitude = -3; aptitude <= 3; aptitude++) {
            CAPTURE(base_level);
            CAPTURE(aptitude);
            CHECK_EQ(derive_level(base_level, aptitude), expected[aptitude + 3]);
        }
    }

    // 基準が 50 の魔法は段階によらず動かない
    CHECK_EQ(50, derive_level(50, -9, LEVEL_ONLY_STEPS));
    CHECK_EQ(50, derive_level(50, 9, LEVEL_ONLY_STEPS));
}

TEST_CASE("derive_magic_by_aptitude rounds the learn level move half away from zero")
{
    // 25 × 25 × 20 / 5000 = 2.5 は 0 から遠い側の 3 に丸める (切り捨てなら 2)
    constexpr AptitudeSteps steps{ 20, 15, 3 };
    CHECK(derive_level(25, 1, steps) == 22);
    CHECK(derive_level(25, -1, steps) == 28);
}

TEST_CASE("derive_magic_by_aptitude clamps the learn level and the difficulty")
{
    // 基準 1: 1 × 49 × 30 × 9 / 5000 = 2.646 なので 3 動く
    CHECK(derive_level(1, -9, LEVEL_ONLY_STEPS) == 4);
    CHECK(derive_level(1, 9, LEVEL_ONLY_STEPS) == 1);
    CHECK(derive_level(49, -9, LEVEL_ONLY_STEPS) == 50);

    const auto easy = derive_magic_by_aptitude(make_base(10, 10, 5, 1), DEFAULT_STEPS, 3);
    REQUIRE(easy.has_value());
    CHECK(easy->sfail == 0);
    const auto hard = derive_magic_by_aptitude(make_base(10, 10, 98, 1), DEFAULT_STEPS, -3);
    REQUIRE(hard.has_value());
    CHECK(hard->sfail == 100);
}

TEST_CASE("derive_magic_by_aptitude rounds the mana cost and keeps it at least 1")
{
    const auto check_mana = [](int base_mana, int aptitude, int expected) {
        CAPTURE(base_mana);
        CAPTURE(aptitude);
        const auto derived = derive_magic_by_aptitude(make_base(10, base_mana, 50, 1), DEFAULT_STEPS, aptitude);
        REQUIRE(derived.has_value());
        CHECK(derived->smana == expected);
    };
    check_mana(10, 2, 7); // 10 × 70% = 7
    check_mana(10, -3, 15); // 10 × 145% = 14.5 は 15 に丸める
    check_mana(3, 1, 3); // 3 × 85% = 2.55 は 3 に丸める
    check_mana(1, 3, 1); // 1 × 55% = 0.55 は 1 に丸める
    check_mana(1, 6, 1); // 1 × 10% = 0.1 でも 1 は残す
}

TEST_CASE("derive_magic_by_aptitude rejects values out of range")
{
    // 消費MP の倍率が 1% 未満になる段階 (15 × 7 = 105%)
    CHECK_FALSE(derive_magic_by_aptitude(make_base(10, 10, 50, 1), DEFAULT_STEPS, 7).has_value());
    // 消費MP が上限 2047 を超える
    CHECK_FALSE(derive_magic_by_aptitude(make_base(10, 2000, 50, 1), DEFAULT_STEPS, -1).has_value());
    CHECK(derive_magic_by_aptitude(make_base(10, 1780, 50, 1), DEFAULT_STEPS, -1).has_value());
}

TEST_CASE("derive_magic_by_aptitude leaves an unlearnable spell unlearnable")
{
    const auto derived = derive_magic_by_aptitude(magic_type{}, DEFAULT_STEPS, 2);
    REQUIRE(derived.has_value());
    CHECK_FALSE(derived->is_learnable());
}
