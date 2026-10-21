/*!
 * @brief 町の番号と町の ID の変換のテスト
 */

#include "system/floor/town-records.h"
#include <doctest/doctest.h>
#include <stdexcept>

TEST_CASE("to_town_index converts a town ID to a town index")
{
    CHECK(to_town_index(TownId::OUTPOST) == 1);
    CHECK(to_town_index(TownId::RLYEH) == 6);
}

TEST_CASE("to_town_id converts a town index to a town ID")
{
    CHECK(to_town_id(1) == TownId::OUTPOST);
    CHECK(to_town_id(6) == TownId::RLYEH);
    CHECK(to_town_id(to_town_index(TownId::MORIVANT)) == TownId::MORIVANT);
}

TEST_CASE("to_town_index rejects IDs out of range")
{
    CHECK_THROWS_AS(to_town_index(TownId::MAX), std::out_of_range);
}

TEST_CASE("to_town_id rejects indexes without a town ID")
{
    CHECK_THROWS_AS(to_town_id(0), std::out_of_range);
    CHECK_THROWS_AS(to_town_id(static_cast<size_t>(TownId::MAX) + 1), std::out_of_range);
}
