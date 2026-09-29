#include "info-reader/town-definition-list-reader.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>

namespace {
nlohmann::json make_town_definition_list()
{
    return {
        { "version", 1 },
        { "towns", {
                       { "1", { { "lite", "towns/Lite.jsonc" }, { "normal", "towns/Normal.jsonc" }, { "none", "towns/None.jsonc" } } },
                       { "2", "towns/Town2.jsonc" },
                   } },
    };
}
}

TEST_CASE("TownDefinitionListReader selects each mode and a direct map file")
{
    const auto data = nlohmann::json::parse(make_town_definition_list().dump());
    REQUIRE(data["version"].is_number_unsigned());
    TownDefinitionListReader reader(data);
    std::string map_file = "old";
    for (const auto &[mode, expected] : {
             std::pair{ TownMapMode::NORMAL, "towns/Normal.jsonc" },
             std::pair{ TownMapMode::LITE, "towns/Lite.jsonc" },
             std::pair{ TownMapMode::NONE, "towns/None.jsonc" },
         }) {
        CAPTURE(std::string(expected));
        REQUIRE(reader.read(1, mode, map_file) == PARSE_ERROR_NONE);
        CHECK(map_file == expected);
    }
    for (const auto mode : { TownMapMode::NORMAL, TownMapMode::LITE, TownMapMode::NONE }) {
        CHECK(reader.read(2, mode, map_file) == PARSE_ERROR_NONE);
        CHECK(map_file == "towns/Town2.jsonc");
    }
}

TEST_CASE("TownDefinitionListReader leaves output unchanged when a town is absent")
{
    const auto data = make_town_definition_list();
    std::string map_file = "previous";
    TownDefinitionListReader reader(data);
    CHECK(reader.read(3, TownMapMode::NORMAL, map_file) == PARSE_ERROR_NONE);
    CHECK(map_file == "previous");
    CHECK(reader.read(-1, TownMapMode::LITE, map_file) == PARSE_ERROR_NONE);
    CHECK(map_file == "previous");
}

TEST_CASE("TownDefinitionListReader validates the root and preserves output")
{
    for (const auto &root : { nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json::object() }) {
        std::string map_file = "previous";
        CHECK(TownDefinitionListReader(root).read(1, TownMapMode::NORMAL, map_file) == PARSE_ERROR_INVALID_TYPE);
        CHECK(map_file == "previous");
    }
    for (const auto &version : { nlohmann::json(0), nlohmann::json(1.0), nlohmann::json("1") }) {
        auto data = make_town_definition_list();
        data["version"] = version;
        std::string map_file = "previous";
        CHECK(TownDefinitionListReader(data).read(1, TownMapMode::NORMAL, map_file) == PARSE_ERROR_INVALID_TYPE);
        CHECK(map_file == "previous");
    }
    for (const auto &towns : { nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json::object() }) {
        auto data = make_town_definition_list();
        data["towns"] = towns;
        std::string map_file = "previous";
        CHECK(TownDefinitionListReader(data).read(1, TownMapMode::NORMAL, map_file) == PARSE_ERROR_INVALID_TYPE);
        CHECK(map_file == "previous");
    }
}

TEST_CASE("TownDefinitionListReader validates the selected town and mode")
{
    for (const auto &selected : { nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json(2) }) {
        auto data = make_town_definition_list();
        data["towns"]["1"] = selected;
        std::string map_file = "previous";
        CHECK(TownDefinitionListReader(data).read(1, TownMapMode::NORMAL, map_file) == PARSE_ERROR_INVALID_TYPE);
        CHECK(map_file == "previous");
    }
    auto data = make_town_definition_list();
    data["towns"]["1"].erase("lite");
    std::string map_file = "previous";
    CHECK(TownDefinitionListReader(data).read(1, TownMapMode::LITE, map_file) == PARSE_ERROR_INVALID_TYPE);
    CHECK(map_file == "previous");
    CHECK(TownDefinitionListReader(data).read(1, TownMapMode::NORMAL, map_file) == PARSE_ERROR_NONE);
    CHECK(map_file == "towns/Normal.jsonc");
    map_file = "previous";
    CHECK(TownDefinitionListReader(data).read(1, static_cast<TownMapMode>(999), map_file) == PARSE_ERROR_INVALID_VALUE);
    CHECK(map_file == "previous");
}

TEST_CASE("TownDefinitionListReader rejects invalid map paths without replacing output")
{
    for (const auto *path : { "", "towns/Map.txt", "../towns/Map.jsonc", "towns/../Map.jsonc", "towns\\Map.jsonc", "Map.jsonc" }) {
        CAPTURE(std::string(path));
        auto data = make_town_definition_list();
        data["towns"]["1"]["normal"] = path;
        std::string map_file = "previous";
        CHECK(TownDefinitionListReader(data).read(1, TownMapMode::NORMAL, map_file) == PARSE_ERROR_INVALID_VALUE);
        CHECK(map_file == "previous");
    }
    auto data = make_town_definition_list();
    data["towns"]["1"]["normal"] = nullptr;
    std::string map_file = "previous";
    CHECK(TownDefinitionListReader(data).read(1, TownMapMode::NORMAL, map_file) == PARSE_ERROR_INVALID_TYPE);
    CHECK(map_file == "previous");
    data["towns"]["2"] = "towns/../Map.jsonc";
    CHECK(TownDefinitionListReader(data).read(2, TownMapMode::NORMAL, map_file) == PARSE_ERROR_INVALID_VALUE);
    CHECK(map_file == "previous");
}

TEST_CASE("TownDefinitionListReader only checks the selected entry as before")
{
    auto data = make_town_definition_list();
    data["towns"]["2"] = nullptr;
    std::string map_file = "previous";
    CHECK(TownDefinitionListReader(data).read(1, TownMapMode::NORMAL, map_file) == PARSE_ERROR_NONE);
    CHECK(map_file == "towns/Normal.jsonc");
    map_file = "previous";
    CHECK(TownDefinitionListReader(data).read(3, TownMapMode::NORMAL, map_file) == PARSE_ERROR_NONE);
    CHECK(map_file == "previous");
}
