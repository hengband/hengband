/*!
 * @brief Diceクラスのテスト
 *
 * ダイスの生成・文字列との相互変換・出目の計算を検証する。
 * ダメージ計算の土台であり、定義ファイルの読込 (info_set_dice) も
 * Dice::parse に依存しているため、境界値と異常系を重点的に確かめる。
 *
 * @details Dice を返す関数の呼び出しを `== 期待値` と組み合わせて CHECK に直接書くと、
 * MSVCが評価順序に関する警告 (C4866) を出す。この警告はエラーとして扱われるため、
 * 呼び出しの結果は一旦変数で受けてから比較する。
 */

#include "util/dice.h"

#include "test/scoped-rng.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

TEST_CASE("Dice is invalid just after default construction")
{
    const Dice dice;

    CHECK(dice.num == 0);
    CHECK(dice.sides == 0);
    CHECK_FALSE(dice.is_valid());
}

TEST_CASE("Dice holds the number and the sides given")
{
    const Dice dice(3, 5);

    CHECK(dice.num == 3);
    CHECK(dice.sides == 5);
    CHECK(dice.is_valid());
}

TEST_CASE("Dice is valid only when both the number and the sides are positive")
{
    // 1個1面が振れる最小のダイス
    CHECK(Dice(1, 1).is_valid());

    CHECK_FALSE(Dice(0, 6).is_valid());
    CHECK_FALSE(Dice(6, 0).is_valid());
    CHECK_FALSE(Dice(-1, 6).is_valid());
    CHECK_FALSE(Dice(6, -1).is_valid());
}

TEST_CASE("Dice compares equal only when both the number and the sides match")
{
    CHECK(Dice(3, 5) == Dice(3, 5));
    CHECK(Dice(3, 5) != Dice(5, 3));
    CHECK(Dice(3, 5) != Dice(3, 6));
    CHECK(Dice(3, 5) != Dice(4, 5));
}

TEST_CASE("Dice::parse builds a dice from the NdM notation")
{
    const auto parsed = Dice::parse("3d5");
    CHECK(parsed == Dice(3, 5));

    const auto minimum = Dice::parse("1d1");
    CHECK(minimum == Dice(1, 1));

    // 複数桁
    const auto multi_digits = Dice::parse("10d100");
    CHECK(multi_digits == Dice(10, 100));
}

TEST_CASE("Dice::parse rejects a malformed string")
{
    // 'd' で2つに分割できない
    CHECK_THROWS_AS(Dice::parse("3"), std::runtime_error);
    CHECK_THROWS_AS(Dice::parse("35"), std::runtime_error);
    CHECK_THROWS_AS(Dice::parse("3d5d7"), std::runtime_error);

    // 'd' の前後が整数として読めない
    CHECK_THROWS_AS(Dice::parse("3d"), std::runtime_error);
    CHECK_THROWS_AS(Dice::parse("d5"), std::runtime_error);
    CHECK_THROWS_AS(Dice::parse("d"), std::runtime_error);
    CHECK_THROWS_AS(Dice::parse("dice"), std::runtime_error);

    // 空文字列
    CHECK_THROWS_AS(Dice::parse(""), std::runtime_error);

    // intに収まらない値。std::stoi の std::out_of_range も同じく変換される
    CHECK_THROWS_AS(Dice::parse("99999999999d6"), std::runtime_error);
}

TEST_CASE("Dice::parse does not check whether the dice is valid")
{
    // parse が見るのは 'd' で2つに分割でき、前後がそれぞれ整数として読めることまで。
    // 振れる値かどうかは is_valid() で別に判定する契約になっている。
    // 定義ファイルには実際に "0d0" が書かれている (BaseitemDefinitions.jsonc) ため、
    // parse が非正値を拒否するようにすると定義ファイルの読込が失敗する
    const auto no_dice = Dice::parse("0d0");
    CHECK(no_dice == Dice(0, 0));
    CHECK_FALSE(no_dice.is_valid());

    const auto zero_num = Dice::parse("0d6");
    CHECK(zero_num == Dice(0, 6));
    CHECK_FALSE(zero_num.is_valid());

    const auto negative_num = Dice::parse("-1d6");
    CHECK(negative_num == Dice(-1, 6));
    CHECK_FALSE(negative_num.is_valid());
}

TEST_CASE("Dice::to_string builds the NdM notation")
{
    CHECK(Dice::to_string(3, 5) == "3d5");
    CHECK(Dice(10, 100).to_string() == "10d100");
}

TEST_CASE("Dice survives a round trip through its string notation")
{
    for (const auto &dice : { Dice(1, 1), Dice(3, 5), Dice(10, 100) }) {
        CAPTURE(dice.to_string());

        const auto restored = Dice::parse(dice.to_string());
        CHECK(restored == dice);
    }
}

TEST_CASE("Dice::maxroll returns the sum of the maximum pips")
{
    CHECK(Dice::maxroll(3, 5) == 15);
    CHECK(Dice(3, 5).maxroll() == 15);
    CHECK(Dice(1, 1).maxroll() == 1);
}

TEST_CASE("Dice::expected_value returns the average of the sum")
{
    // 1d6 の期待値は 3.5 で、整数では表せない
    CHECK(Dice::expected_value(1, 6) == doctest::Approx(3.5));
    CHECK(Dice(1, 6).expected_value() == doctest::Approx(3.5));

    CHECK(Dice(3, 5).expected_value() == doctest::Approx(9.0));
    CHECK(Dice(1, 1).expected_value() == doctest::Approx(1.0));
}

TEST_CASE("Dice::floored_expected_value truncates the fraction of the average")
{
    // 1d2 の期待値 1.5 の小数部を切り捨てて 1
    CHECK(Dice(1, 2).floored_expected_value() == 1);

    // 割り切れる場合は期待値と一致する
    CHECK(Dice(3, 5).floored_expected_value() == 9);
    CHECK(Dice(1, 1).floored_expected_value() == 1);
}

TEST_CASE("Dice::floored_expected_value multiplies before truncating")
{
    // 倍率は切り捨ての前に掛ける。1.5 * 2 = 3 であり、
    // 切り捨ててから掛けた 1 * 2 = 2 にはならない
    CHECK(Dice(1, 2).floored_expected_value_multiplied_by(2) == 3);
    CHECK(Dice::floored_expected_value(1, 2, 2) == 3);

    // 倍率1は floored_expected_value() と同じ
    CHECK(Dice(1, 2).floored_expected_value_multiplied_by(1) == Dice(1, 2).floored_expected_value());

    CHECK(Dice(3, 5).floored_expected_value_multiplied_by(10) == 90);
}

namespace {

//! ダイスを指定回数振り、出目の合計の最小値と最大値を返す
std::pair<int, int> roll_repeatedly(const Dice &dice, int count)
{
    auto min_result = std::numeric_limits<int>::max();
    auto max_result = std::numeric_limits<int>::min();

    for (auto i = 0; i < count; i++) {
        const auto result = dice.roll();
        min_result = std::min(min_result, result);
        max_result = std::max(max_result, result);
    }

    return { min_result, max_result };
}

}

TEST_CASE("Dice::roll stays within the range of the possible sums")
{
    const auto restore_rng = test::scoped_rng();

    for (const auto &dice : { Dice(1, 1), Dice(1, 6), Dice(3, 5), Dice(10, 100) }) {
        CAPTURE(dice.to_string());

        // 構造化束縛の変数は CAPTURE (ラムダで対象を包む) に渡せない処理系
        // (clang 15以前) があるため、通常の変数で受ける
        const auto results = roll_repeatedly(dice, 1000);
        const auto min_result = results.first;
        const auto max_result = results.second;

        CHECK(min_result >= dice.num);
        CHECK(max_result <= dice.maxroll());
    }
}

TEST_CASE("Dice::roll reaches both ends of its range")
{
    const auto restore_rng = test::scoped_rng();

    // 十分な回数振れば両端の出目が出る。片方しか出ない場合は
    // 出目の範囲が1つずれている
    const Dice dice(1, 6);
    const auto results = roll_repeatedly(dice, 1000);
    const auto min_result = results.first;
    const auto max_result = results.second;

    CHECK(min_result == 1);
    CHECK(max_result == dice.sides);
}

TEST_CASE("Dice::roll of a single sided dice always returns the number of the dice")
{
    const auto restore_rng = test::scoped_rng();

    CHECK(Dice(1, 1).roll() == 1);
    CHECK(Dice(5, 1).roll() == 5);
    CHECK(Dice::roll(5, 1) == 5);
}
