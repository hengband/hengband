/*!
 * @brief PlayerCutクラスのテスト
 *
 * 切り傷のランク判定、ランクに応じた出血ダメージ・表示色、
 * モンスター打撃による切り傷値の蓄積を検証する。
 * 蓄積値の算出は乱数を用いるため、乱数シードを固定したうえで値域を検証する。
 */

#include "timed-effect/player-cut.h"

#include "term/term-color-types.h"
#include "test/scoped-rng.h"
#include "util/enum-converter.h"

#include <doctest/doctest.h>

#include <limits>
#include <stdexcept>

namespace {

//! 乱数を用いる蓄積値の検証で行う試行回数
constexpr auto ACCUMULATION_TRIAL_COUNT = 50;

//! 切り傷のランクと、その境界値・ランクに応じた値の対応
struct CutRankExpectation {
    short value_min; //!< このランクになる最小の切り傷値
    short value_max; //!< このランクになる最大の切り傷値
    PlayerCutRank rank;
    int damage; //!< 1ターンあたりの出血ダメージ
    term_color_type color; //!< ステータス表示の色
};

constexpr CutRankExpectation CUT_RANKS[] = {
    { 0, 0, PlayerCutRank::NONE, 0, TERM_WHITE },
    { 1, 10, PlayerCutRank::GRAZING, 1, TERM_YELLOW },
    { 11, 25, PlayerCutRank::LIGHT, 3, TERM_YELLOW },
    { 26, 50, PlayerCutRank::BAD, 7, TERM_ORANGE },
    { 51, 100, PlayerCutRank::NASTY, 16, TERM_ORANGE },
    { 101, 200, PlayerCutRank::SEVERE, 32, TERM_RED },
    { 201, 1000, PlayerCutRank::DEEP, 80, TERM_RED },
    { 1001, std::numeric_limits<short>::max(), PlayerCutRank::MORTAL, 200, TERM_L_RED },
};

}

TEST_CASE("PlayerCut::get_rank returns the rank for the boundary value")
{
    for (const auto &expectation : CUT_RANKS) {
        for (const auto value : { expectation.value_min, expectation.value_max }) {
            CAPTURE(value);
            CHECK(PlayerCut::get_rank(value) == expectation.rank);
        }
    }
}

// 表示文字列は _() マクロで日本語版と英語版が切り替わるため、色だけを検証する
TEST_CASE("PlayerCut reports the rank of the value set with its damage and color")
{
    PlayerCut player_cut;

    for (const auto &expectation : CUT_RANKS) {
        CAPTURE(expectation.value_min);
        player_cut.set(expectation.value_min);

        CHECK(player_cut.get_rank() == expectation.rank);
        CHECK(player_cut.get_damage() == expectation.damage);
        CHECK(std::get<0>(player_cut.get_expr()) == expectation.color);
    }
}

TEST_CASE("PlayerCut::get_cut_mes rejects an invalid rank")
{
    CHECK_THROWS_AS(PlayerCut::get_cut_mes(i2enum<PlayerCutRank>(99)), std::logic_error);
}

TEST_CASE("PlayerCut::get_accumulation gives no cut for a weak hit")
{
    // ダメージが最大ダメージの19/20未満ならば、乱数を引くまでもなく切り傷を負わない
    CHECK(PlayerCut::get_accumulation(100, 50) == 0);
}

TEST_CASE("PlayerCut::get_accumulation returns a value within the range of the rank")
{
    const auto restore_rng = test::scoped_rng();

    // damage >= total かつ damage >= 40 かつ damage > 45 なので、蓄積ランクは乱数によらず必ず7以上
    CHECK(PlayerCut::get_accumulation(46, 46) == 500);

    // damage > 25 なので蓄積ランクは4以上。one_in_(50) で更に上がる場合があるため上限は500
    for (auto i = 0; i < ACCUMULATION_TRIAL_COUNT; i++) {
        const auto accumulation = PlayerCut::get_accumulation(26, 26);
        CHECK(accumulation >= 51);
        CHECK(accumulation <= 500);
    }
}
