#include "info-reader/general-parser.h"
#include "info-reader/legend-ego-validator.h"
#include "info-reader/random-grid-effect-types.h"
#include "object-enchant/trg-types.h"
#include "test/info-reader/legend-ego-fixture.h"
#include <doctest/doctest.h>
#include <initializer_list>

TEST_CASE("Legend ego validator accepts absent ego without an object")
{
    const dungeon_grid grid{};
    CHECK(validate_legend_ego(grid, true).has_value());
}

TEST_CASE("Legend ego validator reports each placement error")
{
    const test::LegendEgoFixture fixture;
    dungeon_grid grid{};
    grid.object = 42;
    grid.ego = EgoType::DF;
    CHECK(validate_legend_ego(grid, false).has_value());
    const auto check = [&](LegendEgoError expected, bool reward = false) {
        const auto result = validate_legend_ego(grid, reward);
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == expected);
    };
    grid.ego = static_cast<EgoType>(1);
    check(LegendEgoError::UNDEFINED_ID);
    grid.ego = EgoType::MORGUL;
    check(LegendEgoError::WORTHLESS);
    grid.ego = EgoType::DF;
    egos_info.at(EgoType::DF).gen_flags.set(ItemGenerationTraitType::CURSED);
    check(LegendEgoError::WORTHLESS);
    egos_info.at(EgoType::DF).gen_flags.clear();
    grid.object = 0;
    check(LegendEgoError::NO_PLACED_OBJECT);
    grid.object = 600;
    check(LegendEgoError::NO_PLACED_OBJECT);
    grid.object = 42;
    grid.random = RANDOM_OBJECT;
    check(LegendEgoError::RANDOM_OBJECT);
    grid.random = 0;
    check(LegendEgoError::QUEST_REWARD, true);
    grid.object = 56;
    check(LegendEgoError::GOLD);
    grid.object = 42;
    grid.ego = EgoType::NONE;
    grid.random = RANDOM_EGO;
    CHECK(validate_legend_ego(grid, false).has_value());
    grid.object = 54;
    check(LegendEgoError::INCOMPATIBLE);
}

TEST_CASE("Legend ego validator follows strict equipment compatibility")
{
    const test::LegendEgoFixture fixture;
    struct Case {
        short object;
        EgoType ego;
        bool allowed;
    };
    const Case cases[]{
        { 42, EgoType::DF, true },
        { 42, EgoType::EARTHQUAKES, false },
        { 43, EgoType::SHARPNESS, false },
        { 94, EgoType::TELEPATHY, false },
        { 48, EgoType::TELEPATHY, true },
        { 94, EgoType::SEEING, true },
        { 122, EgoType::REFLECTION, false },
        { 101, EgoType::TWILIGHT, true },
        { 84, EgoType::DIGGING, true },
        { 84, EgoType::DF, false },
        { 42, EgoType::DIGGING, false },
        { 582, EgoType::LITE_LONG, false },
        { 413, EgoType::SEEING, false },
        { 414, EgoType::ENDURE_FIRE, false },
        { 54, EgoType::DF, false },
        { 55, EgoType::DF, false },
    };
    for (const auto &entry : cases) {
        CAPTURE(entry.object);
        CAPTURE(static_cast<int>(entry.ego));
        dungeon_grid grid{};
        grid.object = entry.object;
        grid.ego = entry.ego;
        const auto result = validate_legend_ego(grid, false);
        CHECK(result.has_value() == entry.allowed);
        if (!entry.allowed) {
            REQUIRE_FALSE(result.has_value());
            CHECK(result.error() == LegendEgoError::INCOMPATIBLE);
        }
    }
    for (const auto object : std::initializer_list<short>{ 42, 84, 94, 48, 101, 122, 582, 413, 414, 54, 55 }) {
        dungeon_grid grid{};
        grid.object = object;
        grid.random = RANDOM_EGO;
        CHECK(validate_legend_ego(grid, false).has_value() == (object != 413 && object != 414 && object != 54 && object != 55));
    }
}

TEST_CASE("Legend ego diagnostic includes fixed or random ego and object ID")
{
    dungeon_grid grid{};
    grid.ego = EgoType::DF;
    grid.object = 54;
    CHECK(describe_legend_ego_spec(grid, false) == "ego 65 (object 54)");
    grid.random |= RANDOM_EGO;
    CHECK(describe_legend_ego_spec(grid, false) == "ego random (object 54)");
}

TEST_CASE("Legend ego diagnostic names random and quest reward objects instead of their numbers")
{
    dungeon_grid grid{};
    grid.ego = EgoType::DF;

    // ランダムな品の object は生成階層の補正であり、品IDとして表示しない。
    grid.random = RANDOM_OBJECT;
    grid.object = 12;
    CHECK(describe_legend_ego_spec(grid, false) == "ego 65 (object random)");
    grid.random |= RANDOM_EGO;
    CHECK(describe_legend_ego_spec(grid, false) == "ego random (object random)");

    // 報酬の品の object は 0 のままであり、品IDとして表示しない。
    grid.random = 0;
    grid.object = 0;
    CHECK(describe_legend_ego_spec(grid, true) == "ego 65 (object questReward)");

    // 固定の品は品IDを表示する。
    grid.object = 42;
    CHECK(describe_legend_ego_spec(grid, false) == "ego 65 (object 42)");
}
