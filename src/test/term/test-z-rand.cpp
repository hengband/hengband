/*!
 * @brief 乱数ユーティリティのテスト
 *
 * term/z-rand.h で宣言されている乱数関数を検証する。
 *
 * 分布の変換は自前のアルゴリズムで実装しており、同じシードからはプラットフォームや
 * 標準ライブラリの実装によらず同じ乱数列が得られなければならない。これを、あらかじめ求めておいた
 * 期待値 (ゴールデン値) との比較で検証する。分布のアルゴリズムを意図して変更した場合は
 * ゴールデン値も更新すること。
 *
 * なお rand_range() のゴールデン値は、変更前の実装 (std::uniform_int_distribution) で
 * MSVC と libstdc++ が出力していた値と一致している。
 */

#include "term/z-rand.h"

#include "system/angband-system.h"
#include "test/scoped-rng.h"
#include "util/enum-range.h"
#include "util/probability-table.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdint>
#include <map>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

/*!
 * @brief 関数を指定回数呼び出した結果の並びを返す
 * @param count 呼び出す回数
 * @param func 呼び出す関数
 * @return 結果の並び
 */
template <typename Func>
auto generate(int count, Func func)
{
    std::vector<decltype(func())> result;
    for (auto i = 0; i < count; ++i) {
        result.push_back(func());
    }

    return result;
}

/*!
 * @brief ゲームの乱数生成器の現在の状態を取得する
 * @return 乱数生成器の状態
 */
auto get_rng_state()
{
    std::array<uint32_t, xso::rng32::word_count()> state{};
    AngbandSystem::get_instance().get_rng().get_state(state.begin());
    return state;
}

enum class TestEnum {
    ZERO,
    ONE,
    TWO,
    THREE,
};

}

TEST_CASE("Random sequences from a fixed seed are platform-independent")
{
    const auto restore_rng = test::scoped_rng();

    SUBCASE("rand_range")
    {
        const std::vector<int> expected{ 86, 30, 91, 80, 64, 99, 99, 37, 33, 46, 1, 59, 94, 32, 95, 25, 97, 28, 65, 43 };
        CHECK(generate(20, [] { return rand_range(1, 100); }) == expected);
    }

    SUBCASE("randnor")
    {
        const std::vector<int16_t> expected{ 150, 75, 98, 103, 159, 109, 121, 128, 144, 60, 85, 88, 81, 105, 107, 74, 141, 132, 45, 125 };
        CHECK(generate(20, [] { return randnor(100, 25); }) == expected);
    }

    SUBCASE("rand_shuffle")
    {
        std::vector<int> values(20);
        std::iota(values.begin(), values.end(), 0);
        rand_shuffle(values.begin(), values.end());
        const std::vector<int> expected{ 6, 15, 9, 11, 1, 8, 2, 7, 18, 0, 12, 3, 4, 19, 14, 10, 13, 16, 5, 17 };
        CHECK(values == expected);
    }

    SUBCASE("ProbabilityTable")
    {
        ProbabilityTable<int> table;
        table.entry_item(0, 1);
        table.entry_item(1, 2);
        table.entry_item(2, 3);
        const std::vector<int> expected{ 2, 1, 2, 2, 2, 2, 2, 1, 1, 1, 0, 2, 2, 1, 2, 1, 2, 1, 2, 1 };
        CHECK(generate(20, [&table] { return table.pick_one_at_random(); }) == expected);
    }
}

TEST_CASE("rand_range returns a uniformly distributed value in [a, b]")
{
    const auto restore_rng = test::scoped_rng();

    SUBCASE("a >= b returns a without consuming the RNG")
    {
        const auto state = get_rng_state();
        CHECK(rand_range(5, 5) == 5);
        CHECK(rand_range(5, 3) == 5);
        CHECK(get_rng_state() == state);
    }

    SUBCASE("values are in the range")
    {
        for (auto i = 0; i < 10000; ++i) {
            const auto value = rand_range(-5, 5);
            REQUIRE(value >= -5);
            REQUIRE(value <= 5);
        }
    }

    SUBCASE("the full range of int is accepted")
    {
        auto has_negative = false;
        auto has_positive = false;
        for (auto i = 0; i < 100; ++i) {
            const auto value = rand_range(INT_MIN, INT_MAX);
            has_negative |= value < 0;
            has_positive |= value > 0;
        }

        CHECK(has_negative);
        CHECK(has_positive);
    }

    SUBCASE("every value appears with the same probability")
    {
        constexpr auto count = 60000;
        std::map<int, int> frequencies;
        for (auto i = 0; i < count; ++i) {
            frequencies[rand_range(0, 5)]++;
        }

        REQUIRE(frequencies.size() == 6);
        // 出現回数は二項分布に従うため、その標準偏差の6倍を許容誤差とする
        const auto allowable_error = 6.0 * std::sqrt(count * (1.0 / 6) * (5.0 / 6));
        for (const auto &[value, frequency] : frequencies) {
            CAPTURE(value);
            CHECK(std::abs(frequency - count / 6.0) <= allowable_error);
        }
    }
}

TEST_CASE("randnum0, randnum1 and their variants return values in the documented range")
{
    const auto restore_rng = test::scoped_rng();

    CHECK(randnum0<int>(0) == 0);
    CHECK(randnum1<int>(0) == 1);
    CHECK(randnum1<int>(1) == 1);
    CHECK(randnum1<int>(-1) == -1);
    CHECK(randint0(1) == 0);

    for (auto i = 0; i < 1000; ++i) {
        const auto r0_positive = randint0(5);
        REQUIRE((r0_positive >= 0 && r0_positive < 5));
        const auto r0_negative = randnum0<int>(-5);
        REQUIRE((r0_negative > -5 && r0_negative <= 0));
        const auto r1_positive = randint1(5);
        REQUIRE((r1_positive >= 1 && r1_positive <= 5));
        const auto r1_negative = randnum1<int>(-5);
        REQUIRE((r1_negative >= -5 && r1_negative <= -1));
        const auto spread = rand_spread(10, -3);
        REQUIRE((spread >= 7 && spread <= 13));
        const auto enum_value = randnum0<TestEnum>(TestEnum::THREE);
        REQUIRE((enum_value >= TestEnum::ZERO && enum_value < TestEnum::THREE));
    }

    CHECK(one_in_(1));
    CHECK(one_in_(0));
    CHECK_FALSE(evaluate_percent(0));
    CHECK(evaluate_percent(100));
}

TEST_CASE("randnor returns a value approximately following the normal distribution")
{
    const auto restore_rng = test::scoped_rng();

    SUBCASE("stand <= 0 returns mean without consuming the RNG")
    {
        const auto state = get_rng_state();
        CHECK(randnor(100, 0) == 100);
        CHECK(randnor(-100, -5) == -100);
        CHECK(randnor(100000, 0) == INT16_MAX);
        CHECK(get_rng_state() == state);
    }

    SUBCASE("mean and standard deviation match the arguments")
    {
        constexpr auto count = 100000;
        constexpr auto mean = 50;
        constexpr auto stand = 1000;
        auto sum = 0.0;
        auto square_sum = 0.0;
        auto max_deviation = 0;
        for (auto i = 0; i < count; ++i) {
            const auto value = randnor(mean, stand);
            sum += value;
            square_sum += static_cast<double>(value) * value;
            max_deviation = std::max(max_deviation, std::abs(value - mean));
        }

        const auto sample_mean = sum / count;
        const auto sample_stand = std::sqrt(square_sum / count - sample_mean * sample_mean);
        // 標本平均の標準誤差は stand / sqrt(count)、標本標準偏差の標準誤差はおよそ stand / sqrt(2 * count)
        CHECK(std::abs(sample_mean - mean) <= 6.0 * stand / std::sqrt(count));
        CHECK(std::abs(sample_stand - stand) <= 6.0 * stand / std::sqrt(2.0 * count));
        // 平均±6σで打ち切られる
        CHECK(max_deviation <= 6 * stand);
    }

    SUBCASE("the result is clamped to the range of int16_t")
    {
        auto has_max = false;
        auto has_min = false;
        for (auto i = 0; i < 1000; ++i) {
            const auto value = randnor(0, 1000000);
            has_max |= value == INT16_MAX;
            has_min |= value == INT16_MIN;
        }

        CHECK(has_max);
        CHECK(has_min);
    }
}

TEST_CASE("rand_shuffle permutes the elements")
{
    const auto restore_rng = test::scoped_rng();

    std::vector<int> values(100);
    std::iota(values.begin(), values.end(), 0);
    auto shuffled = values;
    rand_shuffle(shuffled.begin(), shuffled.end());

    CHECK(shuffled != values);
    CHECK(std::is_permutation(shuffled.begin(), shuffled.end(), values.begin()));

    std::vector<int> empty;
    rand_shuffle(empty.begin(), empty.end());
    CHECK(empty.empty());
}

TEST_CASE("rand_choice chooses an element of the range")
{
    const auto restore_rng = test::scoped_rng();

    SUBCASE("container")
    {
        const std::vector<int> values{ 3, 1, 4, 1, 5 };
        for (auto i = 0; i < 100; ++i) {
            REQUIRE(std::ranges::find(values, rand_choice(values)) != values.end());
        }
    }

    SUBCASE("returns a reference to the element")
    {
        std::vector<int> values{ 1 };
        rand_choice(values) = 2;
        CHECK(values.front() == 2);
    }

    SUBCASE("borrowed range")
    {
        constexpr int values[]{ 1, 2, 3 };
        const auto value = rand_choice(std::span(values));
        CHECK((value >= 1 && value <= 3));
    }

    SUBCASE("EnumRange")
    {
        constexpr auto range = EnumRange(TestEnum::ONE, TestEnum::THREE);
        for (auto i = 0; i < 100; ++i) {
            const auto value = rand_choice(range);
            REQUIRE((value == TestEnum::ONE || value == TestEnum::TWO));
        }
    }

    SUBCASE("initializer_list")
    {
        const auto value = rand_choice({ 10, 20, 30 });
        CHECK((value == 10 || value == 20 || value == 30));
    }

    SUBCASE("empty range throws an exception")
    {
        const std::vector<int> empty;
        CHECK_THROWS_AS(rand_choice(empty), std::out_of_range);
    }
}

TEST_CASE("div_round rounds the quotient randomly")
{
    const auto restore_rng = test::scoped_rng();

    CHECK(div_round(7, 0) == 7);
    CHECK(div_round(10, 5) == 2);
    CHECK(div_round(-10, 5) == -2);
    CHECK(div_round(INT32_MIN, INT32_MIN) == 1);
    CHECK(div_round(INT32_MIN, -1) == INT32_MAX);

    for (auto i = 0; i < 100; ++i) {
        const auto positive = div_round(7, 2);
        REQUIRE((positive == 3 || positive == 4));
        const auto negative = div_round(-7, 2);
        REQUIRE((negative == -3 || negative == -4));
        const auto negative_divisor = div_round(7, -2);
        REQUIRE((negative_divisor == -3 || negative_divisor == -4));
    }

    // 期待値が n / d に一致する
    constexpr auto count = 40000;
    auto sum = 0;
    for (auto i = 0; i < count; ++i) {
        sum += div_round(1, 4);
    }

    CHECK(std::abs(static_cast<double>(sum) / count - 0.25) <= 6.0 * std::sqrt(0.25 * 0.75 / count));
}

TEST_CASE("Rand_external does not consume the game RNG")
{
    const auto restore_rng = test::scoped_rng();

    const auto state = get_rng_state();
    for (auto i = 0; i < 100; ++i) {
        const auto value = Rand_external(10);
        REQUIRE((value >= 0 && value < 10));
    }

    CHECK(Rand_external(0) == 0);
    CHECK(Rand_external(-1) == 0);
    CHECK(get_rng_state() == state);
}
