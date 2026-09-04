/*!
 * @brief PlayerStunクラスのテスト
 *
 * 朦朧のランク判定、ランクに応じた各種ペナルティ、
 * モンスター打撃による朦朧値の蓄積を検証する。
 * 蓄積値の算出は乱数を用いるため、乱数シードを固定したうえで値域を検証する。
 */

#include "timed-effect/player-stun.h"

#include "term/term-color-types.h"
#include "test/scoped-rng.h"
#include "util/enum-converter.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {

//! 乱数を用いる蓄積値の検証で行う試行回数
constexpr auto ACCUMULATION_TRIAL_COUNT = 30;

//! 朦朧のランクと、その境界値・ランクに応じた値の対応
struct StunRankExpectation {
    short value_min; //!< このランクになる最小の朦朧値
    short value_max; //!< このランクになる最大の朦朧値
    PlayerStunRank rank;
    int magic_chance_penalty; //!< 魔法系の失率上昇
    int item_chance_penalty; //!< アイテム使用の失率上昇
    short damage_penalty; //!< ダメージ量 or 命中率の低下
    term_color_type color; //!< ステータス表示の色
};

constexpr StunRankExpectation STUN_RANKS[] = {
    { 0, 0, PlayerStunRank::NONE, 0, 0, 0, TERM_WHITE },
    { 1, 50, PlayerStunRank::SLIGHT, 10, 0, 5, TERM_WHITE },
    { 51, 100, PlayerStunRank::NORMAL, 20, 0, 10, TERM_YELLOW },
    { 101, 150, PlayerStunRank::HARD, 30, 5, 20, TERM_ORANGE },
    { 151, 200, PlayerStunRank::UNCONSCIOUS, 50, 10, 40, TERM_RED },
    { 201, std::numeric_limits<short>::max(), PlayerStunRank::KNOCKED, 100, 100, 100, TERM_VIOLET },
};

}

TEST_CASE("PlayerStun::get_rank returns the rank for the boundary value")
{
    for (const auto &expectation : STUN_RANKS) {
        for (const auto value : { expectation.value_min, expectation.value_max }) {
            CAPTURE(value);
            CHECK(PlayerStun::get_rank(value) == expectation.rank);
        }
    }
}

// 表示文字列は _() マクロで日本語版と英語版が切り替わるため、色だけを検証する
TEST_CASE("PlayerStun reports the rank of the value set with its penalties and color")
{
    PlayerStun player_stun;

    for (const auto &expectation : STUN_RANKS) {
        CAPTURE(expectation.value_min);
        player_stun.set(expectation.value_min);

        CHECK(player_stun.get_rank() == expectation.rank);
        CHECK(player_stun.get_magic_chance_penalty() == expectation.magic_chance_penalty);
        CHECK(player_stun.get_item_chance_penalty() == expectation.item_chance_penalty);
        CHECK(player_stun.get_damage_penalty() == expectation.damage_penalty);
        CHECK(std::get<0>(player_stun.get_expr()) == expectation.color);
    }
}

// 共通化により is_stunned() (ランク経由) が基底の is_active() (値が正か) に置き換わる。
// 両者が食い違わないことを、NONE と SLIGHT の境界で確認する。
TEST_CASE("PlayerStun is active exactly when its rank is above NONE")
{
    PlayerStun player_stun;

    CHECK(player_stun.is_active() == (player_stun.get_rank() > PlayerStunRank::NONE));

    player_stun.set(1);
    CHECK(player_stun.is_active() == (player_stun.get_rank() > PlayerStunRank::NONE));
}

TEST_CASE("PlayerStun is knocked out only above the KNOCKED threshold")
{
    PlayerStun player_stun;

    CHECK_FALSE(player_stun.is_knocked_out());

    player_stun.set(200);
    CHECK_FALSE(player_stun.is_knocked_out());

    player_stun.set(201);
    CHECK(player_stun.is_knocked_out());
}

TEST_CASE("PlayerStun::get_stun_mes rejects an invalid rank")
{
    CHECK_THROWS_AS(PlayerStun::get_stun_mes(i2enum<PlayerStunRank>(99)), std::logic_error);
}

// 朦朧の蓄積ランクは乱数を使わない純粋関数なので、境界値をそのまま検証できる
TEST_CASE("PlayerStun::get_accumulation_rank returns the rank for the boundary damage")
{
    // 最大ダメージの19/20未満の打撃では朦朧しない
    CHECK(PlayerStun::get_accumulation_rank(100, 50) == 0);

    // ダメージ20以下では朦朧しない
    CHECK(PlayerStun::get_accumulation_rank(20, 20) == 0);

    // これ以降は damage == total とすることで 19/20 の判定を通し、ダメージ量だけを見る
    struct DamageToRank {
        int damage;
        int rank;
    };

    constexpr DamageToRank DAMAGE_TO_RANK[] = {
        { 21, 1 },
        { 36, 1 },
        { 37, 2 },
        { 51, 2 },
        { 52, 3 },
        { 66, 3 },
        { 67, 4 },
        { 81, 4 },
        { 82, 5 },
        { 96, 5 },
        { 97, 6 },
        { 111, 6 },
        { 112, 7 },
        { 256, 7 },
        { 257, 8 },
    };

    for (const auto &expectation : DAMAGE_TO_RANK) {
        CAPTURE(expectation.damage);
        CHECK(PlayerStun::get_accumulation_rank(expectation.damage, expectation.damage) == expectation.rank);
    }
}

TEST_CASE("PlayerStun::get_accumulation returns a value within the range of the rank")
{
    const auto restore_rng = test::scoped_rng();

    CHECK(PlayerStun::get_accumulation(0) == 0);

    // ランクnの蓄積値は randnum1(10) + 10 * (n - 1)、すなわち [10n-9, 10n]。
    // ランク8以上は全て同じ値域になるので、ランク9も8として扱う
    for (auto rank = 1; rank <= 9; rank++) {
        CAPTURE(rank);
        const auto capped_rank = std::min(rank, 8);

        for (auto i = 0; i < ACCUMULATION_TRIAL_COUNT; i++) {
            const auto accumulation = PlayerStun::get_accumulation(rank);
            CHECK(accumulation >= 10 * capped_rank - 9);
            CHECK(accumulation <= 10 * capped_rank);
        }
    }
}
