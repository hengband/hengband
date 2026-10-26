#include "player-base/player-class.h"
#include "realm/realm-types.h"
#include "spell/spell-info.h"
#include "system/player-type-definition.h"
#include "term/z-rand.h"
#include "test/scoped-rng.h"
#include <algorithm>
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>

/*!
 * @brief 最大消費MPの熟練度補正と消費魔力減少を確認する
 */
TEST_CASE("mod_need_mana handles configured maximum and signed saved proficiency")
{
    PlayerType player;
    player.pclass = PlayerClassType::MAGE;
    player.realm1 = RealmType::LIFE;
    struct ManaCase {
        int proficiency;
        MANA_POINT normal_cost;
        MANA_POINT reduced_cost;
    };
    // MANA_CONST=2400、達人基準=1400。
    // 通常MP=max(1, (2047*(2400+1400-熟練度)+2399)/2400)。
    // 消費魔力減少時は同じ分子に3を掛け、2400*4で割る（途中では丸めない）。
    // 例: 熟練度0は7780999/2400=3242、減少時は23342997/9600=2431。
    // 保存値-32768は74857095/2400=31190、減少時は224571285/9600=23392。
    const ManaCase cases[] = { { 0, 3242, 2431 }, { 1400, 2047, 1535 }, { 1600, 1877, 1408 }, { -32768, 31190, 23392 }, { 32767, 1, 1 } };
    for (const auto &test_case : cases) {
        for (const auto dec_mana : { false, true }) {
            CAPTURE(test_case.proficiency);
            CAPTURE(dec_mana);
            player.spell_exp[0] = static_cast<int16_t>(test_case.proficiency);
            player.dec_mana = dec_mana;
            const auto expected = dec_mana ? test_case.reduced_cost : test_case.normal_cost;
            CHECK(mod_need_mana(&player, 2047, 0, RealmType::LIFE) == expected);
        }
    }
    player.spell_exp[0] = 0;
    player.dec_mana = false;
    CHECK(mod_need_mana(&player, 0, 0, RealmType::LIFE) == 1);
    CHECK(mod_need_mana(&player, 150, 0, RealmType::LIFE) == 238);
}

/*!
 * @brief 麻痺はintで抽選してから状態異常の上限へ丸め、乱数消費も維持する
 */
TEST_CASE("spell over exertion clamps after rolling without short wraparound")
{
    const auto restore_rng = test::scoped_rng(20261011);
    for (const auto mana : { 0, 999, 2047, 3242, 31190 }) {
        CAPTURE(mana);
        bool saw_capped = false;
        xso::rng32 expected_rng = get_game_rng();
        for (int i = 0; i < 64; ++i) {
            const auto rolled = randint1(expected_rng, 5 * mana + 1);
            const auto actual = roll_spell_over_exertion_duration(mana);
            CHECK(actual == std::min(rolled, 10000));
            CHECK(actual >= 1);
            saw_capped |= actual == 10000;
        }
        if (mana >= 3242) {
            CHECK(saw_capped);
        }
    }
}

TEST_CASE("spell over exertion protects the full input type and preserves RNG draws")
{
    const auto restore_rng = test::scoped_rng(20261013);
    for (const auto mana : { std::numeric_limits<int>::min(), -1, std::numeric_limits<int>::max() }) {
        CAPTURE(mana);
        xso::rng32 expected_rng = get_game_rng();
        const auto maximum = mana < 0 ? 1 : std::numeric_limits<int>::max();
        const auto expected = std::min(randint1(expected_rng, maximum), 10000);
        CHECK(roll_spell_over_exertion_duration(mana) == expected);
        const auto next = randint1(1000);
        CHECK(next == randint1(expected_rng, 1000));
    }
}
