#include "info-reader/town-preferences-reader.h"
#include "system/dungeon/quest-fixed-map.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>

namespace {
nlohmann::json make_town_preferences()
{
    return {
        { "version", 1 },
        { "legend", {
                        { "A", { { "terrain", "FLOOR" }, { "caveInfo", { "MARK" } }, { "special", 1 } } },
                        { "b", { { "terrain", "TREE" }, { "caveInfo", { "MARK", "GLOW" } } } },
                    } },
    };
}

parse_error_type parse_test_cell(const nlohmann::json &data, QuestLegendCell &cell)
{
    const auto &terrain = data["terrain"].get_ref<const std::string &>();
    if (terrain == "INVALID") {
        return PARSE_ERROR_UNDEFINED_TERRAIN_TAG;
    }
    for (const auto &flag : data["caveInfo"]) {
        if (flag == "INVALID_FLAG") {
            return PARSE_ERROR_INVALID_FLAG;
        }
    }
    cell.grid.feature = terrain == "FLOOR" ? 10 : 20;
    cell.grid.cave_info = static_cast<BIT_FLAGS>(data["caveInfo"].size());
    cell.grid.special = static_cast<int16_t>(data.value("special", 0));
    return PARSE_ERROR_NONE;
}

void check_rejected(const nlohmann::json &data, parse_error_type expected)
{
    TownPreferencesLegend legend{ { '?', dungeon_grid{} } };
    legend.front().second.feature = 99;
    CHECK(TownPreferencesReader(data).read(legend, parse_test_cell) == expected);
    REQUIRE(legend.size() == 1);
    CHECK(legend.front().first == '?');
    CHECK(legend.front().second.feature == 99);
}
}

TEST_CASE("TownPreferencesReader reads a complete legend and replaces output")
{
    const auto data = nlohmann::json::parse(make_town_preferences().dump());
    REQUIRE(data["version"].is_number_unsigned());
    TownPreferencesLegend legend{ { '?', dungeon_grid{} } };
    CHECK(TownPreferencesReader(data).read(legend, parse_test_cell) == PARSE_ERROR_NONE);
    REQUIRE(legend.size() == 2);
    CHECK(legend[0].first == 'A');
    CHECK(legend[0].second.feature == 10);
    CHECK(legend[0].second.cave_info == 1);
    CHECK(legend[0].second.special == 1);
    CHECK(legend[1].first == 'b');
    CHECK(legend[1].second.feature == 20);
    CHECK(legend[1].second.cave_info == 2);
    CHECK(legend[1].second.special == 0);
}

TEST_CASE("TownPreferencesReader validates the root and legend")
{
    for (const auto &root : { nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json::object() }) {
        check_rejected(root, PARSE_ERROR_INVALID_TYPE);
    }
    for (const auto &version : { nlohmann::json(0), nlohmann::json(1.0), nlohmann::json("1") }) {
        auto data = make_town_preferences();
        data["version"] = version;
        check_rejected(data, PARSE_ERROR_INVALID_TYPE);
    }
    for (const auto &legend : { nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json::object() }) {
        auto data = make_town_preferences();
        data["legend"] = legend;
        check_rejected(data, PARSE_ERROR_INVALID_TYPE);
    }
}

TEST_CASE("TownPreferencesReader validates printable one-byte symbols")
{
    for (const auto *symbol : { "", "AB", " ", "\xC3\xA9" }) {
        auto data = make_town_preferences();
        data["legend"][symbol] = data["legend"]["A"];
        check_rejected(data, PARSE_ERROR_INVALID_TYPE);
    }
    for (const auto *symbol : { "!", "~" }) {
        auto data = make_town_preferences();
        data["legend"] = { { symbol, { { "terrain", "FLOOR" }, { "caveInfo", nlohmann::json::array() } } } };
        TownPreferencesLegend legend;
        CHECK(TownPreferencesReader(data).read(legend, parse_test_cell) == PARSE_ERROR_NONE);
        REQUIRE(legend.size() == 1);
        CHECK(legend.front().first == symbol[0]);
    }
}

TEST_CASE("TownPreferencesReader validates cell shape and cave flags")
{
    for (const auto &cell : { nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json(1) }) {
        auto data = make_town_preferences();
        data["legend"]["b"] = cell;
        check_rejected(data, PARSE_ERROR_INVALID_TYPE);
    }
    for (const auto &terrain : { nlohmann::json(nullptr), nlohmann::json(1) }) {
        auto data = make_town_preferences();
        data["legend"]["b"]["terrain"] = terrain;
        check_rejected(data, PARSE_ERROR_INVALID_TYPE);
    }
    for (const auto &cave_info : { nlohmann::json(nullptr), nlohmann::json("MARK"), nlohmann::json::object() }) {
        auto data = make_town_preferences();
        data["legend"]["b"]["caveInfo"] = cave_info;
        check_rejected(data, PARSE_ERROR_INVALID_TYPE);
    }
    auto data = make_town_preferences();
    data["legend"]["b"].erase("terrain");
    check_rejected(data, PARSE_ERROR_INVALID_TYPE);
    data = make_town_preferences();
    data["legend"]["b"].erase("caveInfo");
    check_rejected(data, PARSE_ERROR_INVALID_TYPE);
    data = make_town_preferences();
    data["legend"]["b"]["caveInfo"] = { "MARK", 2 };
    check_rejected(data, PARSE_ERROR_INVALID_TYPE);
}

TEST_CASE("TownPreferencesReader validates special within signed 16-bit range")
{
    for (const auto &special : { nlohmann::json(nullptr), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json(-32769), nlohmann::json(32768), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
        auto data = make_town_preferences();
        data["legend"]["b"]["special"] = special;
        check_rejected(data, PARSE_ERROR_INVALID_VALUE);
    }
    for (const auto special : { -32768, 32767 }) {
        auto data = make_town_preferences();
        data["legend"]["b"]["special"] = special;
        TownPreferencesLegend legend;
        CHECK(TownPreferencesReader(data).read(legend, parse_test_cell) == PARSE_ERROR_NONE);
        REQUIRE(legend.size() == 2);
        CHECK(legend[1].second.special == special);
    }
}

TEST_CASE("TownPreferencesReader propagates cell parser errors without partial output")
{
    auto data = make_town_preferences();
    data["legend"]["b"]["terrain"] = "INVALID";
    check_rejected(data, PARSE_ERROR_UNDEFINED_TERRAIN_TAG);
    data = make_town_preferences();
    data["legend"]["b"]["caveInfo"] = { "INVALID_FLAG" };
    check_rejected(data, PARSE_ERROR_INVALID_FLAG);
}

TEST_CASE("TownPreferencesReader retains permissive runtime behavior for extra fields")
{
    auto data = make_town_preferences();
    data["unused"] = true;
    data["legend"]["A"]["unused"] = true;
    TownPreferencesLegend legend;
    CHECK(TownPreferencesReader(data).read(legend, parse_test_cell) == PARSE_ERROR_NONE);
    CHECK(legend.size() == 2);
}
