/*!
 * @brief 突然変異の抽選表のテスト
 *
 * 抽選値 (1～MUTATION_ROLL_MAX) から突然変異を引く表について、以下を検証する。
 *
 * - 抽選値の全範囲に突然変異が割り当てられ、範囲外の値には何も割り当てられないこと
 * - すべての突然変異が、ちょうど1つの連続した区間に割り当てられていること
 * - 呼び出し元が直接指定する抽選値 (トランプ魔法の12と77など) の対応が変わっていないこと
 */

#include "mutation/mutation-roll-table.h"

#include "mutation/mutation-flag-types.h"
#include "util/enum-range.h"

#include <doctest/doctest.h>

#include <set>

TEST_CASE("find_mutation_by_roll finds a mutation for every roll in range")
{
    for (auto roll = 1; roll <= MUTATION_ROLL_MAX; ++roll) {
        CAPTURE(roll);
        const auto entry = find_mutation_by_roll(roll);
        REQUIRE(entry.has_value());
        CHECK(entry->type != PlayerMutationType::MAX);
        CHECK_FALSE(entry->gain_message.empty());
        CHECK_FALSE(entry->lose_message.empty());
    }
}

TEST_CASE("find_mutation_by_roll finds nothing for rolls out of range")
{
    CHECK_FALSE(find_mutation_by_roll(-1).has_value());
    CHECK_FALSE(find_mutation_by_roll(0).has_value());
    CHECK_FALSE(find_mutation_by_roll(MUTATION_ROLL_MAX + 1).has_value());
}

TEST_CASE("every mutation is assigned to exactly one contiguous range of rolls")
{
    // 抽選値の順に突然変異が切り替わるたびに記録する。同じ突然変異が2回現れたら区間が分かれている
    std::set<PlayerMutationType> seen;
    auto prev = PlayerMutationType::MAX;
    for (auto roll = 1; roll <= MUTATION_ROLL_MAX; ++roll) {
        const auto entry = find_mutation_by_roll(roll);
        REQUIRE(entry.has_value());
        if (entry->type != prev) {
            CAPTURE(roll);
            CHECK(seen.insert(entry->type).second);
            prev = entry->type;
        }
    }

    for (const auto type : EnumRange(PlayerMutationType::SPIT_ACID, PlayerMutationType::MAX)) {
        CAPTURE(type);
        CHECK(seen.contains(type));
    }
}

TEST_CASE("find_mutation_by_roll keeps the mapping that callers depend on")
{
    const auto type_of = [](int roll) { return find_mutation_by_roll(roll)->type; };

    CHECK(type_of(1) == PlayerMutationType::SPIT_ACID);
    CHECK(type_of(4) == PlayerMutationType::SPIT_ACID);
    CHECK(type_of(5) == PlayerMutationType::BR_FIRE);

    // トランプ魔法の「生きているカード」が直接指定する値
    CHECK(type_of(12) == PlayerMutationType::VTELEPORT);
    CHECK(type_of(77) == PlayerMutationType::RTELEPORT);

    // 職業や性格によって選ばれない突然変異の区間
    CHECK(type_of(110) == PlayerMutationType::CHAOS_GIFT);
    CHECK(type_of(111) == PlayerMutationType::CHAOS_GIFT);
    CHECK(type_of(188) == PlayerMutationType::BAD_LUCK);
    CHECK(type_of(MUTATION_ROLL_MAX) == PlayerMutationType::GOOD_LUCK);
}
