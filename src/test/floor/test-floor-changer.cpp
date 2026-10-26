#include "floor/floor-changer.h"
#include "system/gamevalue.h"
#include <doctest/doctest.h>
#include <initializer_list>
#include <limits>

TEST_CASE("Loaded floor monster trials are bounded even when every allocation fails")
{
    for (const auto requested : { GAME_TURN{ 200000000 }, std::numeric_limits<GAME_TURN>::max() }) {
        CAPTURE(requested);
        int calls = 0;
        // 制限の回帰時にも1024回で止め、数億回のループをテスト内で実行しない。
        attempt_loaded_floor_monsters(requested, [&calls] { return calls <= MAX_FLOOR_MONSTERS - 1; }, [&calls] {
            ++calls;
            return false; });
        CHECK(calls == MAX_FLOOR_MONSTERS - 1);
    }
}

TEST_CASE("Loaded floor monster trials retain normal counts and temporary failures")
{
    for (const auto requested : { GAME_TURN{ 0 }, GAME_TURN{ 1 }, GAME_TURN{ 20 }, GAME_TURN{ MAX_FLOOR_MONSTERS - 1 } }) {
        CAPTURE(requested);
        int calls = 0;
        attempt_loaded_floor_monsters(requested, [] { return true; }, [&calls] {
            ++calls;
            return false; });
        CHECK(calls == requested);
    }
    CHECK(limit_loaded_floor_monster_trials(-1) == 0);
}

TEST_CASE("Loaded floor monster trials stop when grouped placements fill registration slots")
{
    int monsters = MAX_FLOOR_MONSTERS - 5;
    int calls = 0;
    attempt_loaded_floor_monsters(100, [&monsters] { return monsters < MAX_FLOOR_MONSTERS - 1; }, [&] {
        ++calls;
        monsters += 2;
        return true; });
    CHECK(calls == 2);
    CHECK(monsters == MAX_FLOOR_MONSTERS - 1);
}

TEST_CASE("Loaded floor monster trials do not allocate when registration is already full")
{
    int calls = 0;
    attempt_loaded_floor_monsters(std::numeric_limits<GAME_TURN>::max(), [] { return false; }, [&calls] {
        ++calls;
        return true; });
    CHECK(calls == 0);
}
