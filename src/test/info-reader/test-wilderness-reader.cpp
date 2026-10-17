#include "floor/floor-base-definitions.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/wilderness-reader.h"
#include "locale/language-switcher.h"
#include "system/enums/terrain/wilderness-terrain.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
const std::string TOWN_NAME_JA_UTF8 = "\xe7\x94\xba";

nlohmann::json make_definition()
{
    return {
        { "version", 1 },
        { "width", 3 },
        { "height", 3 },
        { "towns", { { { "id", 1 }, { "name", { { "ja", TOWN_NAME_JA_UTF8 }, { "en", "Town" } } } } } },
        { "maps", {
                      { "normal", {
                                      { "letters", {
                                                       { { "symbol", "#" }, { "terrain", 0 } },
                                                       { { "symbol", "1" }, { "terrain", 1 }, { "level", { { "ja", 30 }, { "en", 20 } } }, { "town", 1 } },
                                                   } },
                                      { "layout", { "###", "#1#", "###" } },
                                      { "starting_position", { { "x", 1 }, { "y", 1 } } },
                                  } },
                      { "compact", {
                                       { "letters", { { { "symbol", "#" }, { "terrain", 0 } } } },
                                       { "layout", { "###", "###", "###" } },
                                       { "starting_position", { { "x", 1 }, { "y", 1 } } },
                                   } },
                  } },
    };
}

WildernessDefinition make_existing_definition()
{
    WildernessDefinition definition;
    definition.width = 99;
    definition.height = 88;
    definition.towns.push_back({ 77, "Previous town", "Previous alias" });
    definition.normal.letters.push_back({ '?', WildernessTerrain::EDGE, 5, 0, 0 });
    definition.normal.layout.push_back("previous normal map");
    definition.compact.layout.push_back("previous compact map");
    return definition;
}

/*!
 * @brief マップ解析失敗後も既存の出力全体が保持されていることを確かめる
 */
void check_existing_definition(const WildernessDefinition &definition)
{
    CHECK(definition.width == 99);
    CHECK(definition.height == 88);
    REQUIRE(definition.towns.size() == 1);
    CHECK(definition.towns[0].id == 77);
    CHECK(definition.towns[0].name == "Previous town");
    CHECK(definition.towns[0].alias == "Previous alias");
    REQUIRE(definition.normal.letters.size() == 1);
    const auto &letter = definition.normal.letters[0];
    CHECK(letter.symbol == '?');
    CHECK(letter.terrain == WildernessTerrain::EDGE);
    CHECK(letter.level == 5);
    CHECK(letter.town == 0);
    CHECK(letter.road == 0);
    CHECK(definition.normal.layout == std::vector<std::string>{ "previous normal map" });
    CHECK(definition.normal.starting_position == Pos2D(0, 0));
    CHECK(definition.compact.letters.empty());
    CHECK(definition.compact.layout == std::vector<std::string>{ "previous compact map" });
    CHECK(definition.compact.starting_position == Pos2D(0, 0));
}

}

TEST_CASE("WildernessReader loads maps and localized levels")
{
    const auto data = make_definition();
    WildernessDefinition definition;
    REQUIRE(WildernessReader(data).read(definition) == PARSE_ERROR_NONE);
    CHECK(definition.width == 3);
    CHECK(definition.height == 3);
    REQUIRE(definition.towns.size() == 1);
    REQUIRE(definition.normal.letters.size() == 2);
    CHECK(definition.normal.letters[1].terrain == WildernessTerrain::TOWN);
    const auto expected_level = definition.towns[0].name == "Town" ? 20 : 30;
    CHECK(definition.normal.letters[1].level == expected_level);
    CHECK(definition.normal.letters[1].town == 1);
    CHECK(definition.normal.starting_position == Pos2D(1, 1));
}

TEST_CASE("WildernessReader rejects duplicate IDs and symbols")
{
    auto data = make_definition();
    data["towns"].push_back(data["towns"][0]);
    WildernessDefinition definition;
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);

    data = make_definition();
    data["maps"]["normal"]["letters"].push_back(data["maps"]["normal"]["letters"][0]);
    definition = {};
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_NON_SEQUENTIAL_RECORDS);
}

TEST_CASE("WildernessReader rejects undefined town references")
{
    for (const auto *map_name : { "normal", "compact" }) {
        auto data = make_definition();
        data["maps"][map_name]["letters"][0]["town"] = 2;
        WildernessDefinition definition;
        CAPTURE(map_name);
        CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_VALUE);
    }
}

TEST_CASE("WildernessReader validates layout dimensions and symbols")
{
    auto data = make_definition();
    data["maps"]["normal"]["layout"][1] = "##";
    WildernessDefinition definition;
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_VALUE);

    data = make_definition();
    data["maps"]["normal"]["layout"][1] = "#.#";
    definition = {};
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_VALUE);

    data = make_definition();
    data["maps"]["compact"]["starting_position"]["x"] = 3;
    definition = {};
    CHECK(WildernessReader(data).read(definition) != PARSE_ERROR_NONE);
}

TEST_CASE("WildernessReader keeps previous output on a late parse error")
{
    auto data = make_definition();
    data["maps"]["compact"]["starting_position"]["x"] = 3;
    auto definition = make_existing_definition();

    CHECK(WildernessReader(data).read(definition) != PARSE_ERROR_NONE);
    check_existing_definition(definition);
}

TEST_CASE("WildernessReader replaces previous output on success")
{
    const auto data = make_definition();
    auto definition = make_existing_definition();

    REQUIRE(WildernessReader(data).read(definition) == PARSE_ERROR_NONE);
    CHECK(definition.width == 3);
    CHECK(definition.height == 3);
    REQUIRE(definition.towns.size() == 1);
    CHECK(definition.towns[0].id == 1);
    REQUIRE(definition.normal.letters.size() == 2);
    REQUIRE(definition.normal.layout.size() == 3);
    CHECK(definition.normal.layout[0] == "###");
    CHECK(definition.normal.layout[1] == "#1#");
    REQUIRE(definition.compact.layout.size() == 3);
    CHECK(definition.compact.layout[0] == "###");
}

/*!
 * @brief 通常・簡易マップの各エラーを伝播し、解析途中の値を出力へ公開しない
 */
TEST_CASE("WildernessReader propagates map helper errors without replacing output")
{
    const auto *selected = _("ja", "en");
    const auto *other = _("en", "ja");
    struct InvalidField {
        const char *path;
        nlohmann::json value;
        parse_error_type error;
    };
    const std::vector<InvalidField> invalid_fields = {
        { "", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "", 1, PARSE_ERROR_INVALID_TYPE },
        { "/letters", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/letters", nlohmann::json::object(), PARSE_ERROR_INVALID_TYPE },
        { "/letters", nlohmann::json::array(), PARSE_ERROR_INVALID_TYPE },
        { "/letters/0", 1, PARSE_ERROR_INVALID_TYPE },
        { "/letters/0/symbol", 1, PARSE_ERROR_INVALID_TYPE },
        { "/letters/0/symbol", "", PARSE_ERROR_INVALID_VALUE },
        { "/letters/0/symbol", "##", PARSE_ERROR_INVALID_VALUE },
        { "/letters/1/symbol", "#", PARSE_ERROR_NON_SEQUENTIAL_RECORDS },
        { "/letters/0/terrain", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/letters/0/terrain", false, PARSE_ERROR_INVALID_TYPE },
        { "/letters/0/terrain", static_cast<int>(WildernessTerrain::MAX), PARSE_ERROR_INVALID_FLAG },
        { "/letters/0/level", "bad", PARSE_ERROR_INVALID_TYPE },
        { "/letters/0/level", false, PARSE_ERROR_INVALID_TYPE },
        { "/letters/0/level", 0.5, PARSE_ERROR_INVALID_TYPE },
        { "/letters/0/level", -1, PARSE_ERROR_INVALID_FLAG },
        { "/letters/0/level", uint64_t{ std::numeric_limits<int>::max() } + 1, PARSE_ERROR_INVALID_FLAG },
        { "/letters/0/level", nlohmann::json::object(), PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/letters/0/level", { { other, 0 } }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/letters/0/level", { { selected, nullptr }, { other, 0 } }, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/letters/0/level", { { selected, false }, { other, 0 } }, PARSE_ERROR_INVALID_TYPE },
        { "/letters/0/level", { { selected, -1 }, { other, 0 } }, PARSE_ERROR_INVALID_FLAG },
        { "/letters/0/town", false, PARSE_ERROR_INVALID_TYPE },
        { "/letters/0/town", 32768, PARSE_ERROR_INVALID_FLAG },
        { "/letters/0/road", 16, PARSE_ERROR_INVALID_FLAG },
        { "/layout", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/layout", nlohmann::json::array(), PARSE_ERROR_INVALID_VALUE },
        { "/layout/0", 1, PARSE_ERROR_INVALID_TYPE },
        { "/layout/0", "", PARSE_ERROR_INVALID_VALUE },
        { "/layout/0", "#X#", PARSE_ERROR_INVALID_VALUE },
        { "/layout/0", "####", PARSE_ERROR_INVALID_VALUE },
        { "/layout/3", "###", PARSE_ERROR_INVALID_VALUE },
        { "/starting_position", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/starting_position", 1, PARSE_ERROR_INVALID_TYPE },
        { "/starting_position/x", nullptr, PARSE_ERROR_TOO_FEW_ARGUMENTS },
        { "/starting_position/x", false, PARSE_ERROR_INVALID_TYPE },
        { "/starting_position/x", 0, PARSE_ERROR_INVALID_FLAG },
        { "/starting_position/y", 3, PARSE_ERROR_INVALID_FLAG },
    };
    for (const auto *map_name : { "normal", "compact" }) {
        CAPTURE(map_name);
        for (const auto &invalid : invalid_fields) {
            CAPTURE(invalid.path);
            CAPTURE(invalid.value.dump());
            auto data = make_definition();
            data[nlohmann::json::json_pointer(std::string("/maps/") + map_name + invalid.path)] = invalid.value;
            auto definition = make_existing_definition();
            CHECK(WildernessReader(data).read(definition) == invalid.error);
            check_existing_definition(definition);
        }
    }

    for (const auto *map_name : { "normal", "compact" }) {
        CAPTURE(map_name);
        auto data = make_definition();
        data["maps"][map_name]["layout"] = { "#", "#", "#" };
        data["maps"][map_name]["starting_position"]["x"] = 1;
        auto definition = make_existing_definition();
        CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_OUT_OF_BOUNDS);
        check_existing_definition(definition);
    }

    auto data = make_definition();
    data["maps"]["normal"]["letters"][0]["terrain"] = false;
    data["maps"]["compact"]["layout"] = nlohmann::json::array();
    auto definition = make_existing_definition();
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_TYPE);
    check_existing_definition(definition);
    data = make_definition();
    data["maps"]["normal"]["layout"].erase(2);
    data["maps"]["compact"]["starting_position"] = false;
    CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_VALUE);
    check_existing_definition(definition);
}

/*!
 * @brief 整数0・上限値・任意の省略値と、選択言語だけを読む既存の契約を維持する
 */
TEST_CASE("WildernessReader preserves direct and localized level boundaries")
{
    const auto *selected = _("ja", "en");
    const auto *other = _("en", "ja");
    const std::vector<nlohmann::json> levels = {
        nullptr,
        0,
        std::numeric_limits<int>::max(),
        { { selected, 0 }, { other, "unused" } },
        { { selected, std::numeric_limits<int>::max() }, { other, false } },
    };
    for (const auto *map_name : { "normal", "compact" }) {
        CAPTURE(map_name);
        for (const auto &level : levels) {
            CAPTURE(level.dump());
            auto data = make_definition();
            data["maps"][map_name]["letters"][0]["level"] = level;
            WildernessDefinition definition;
            REQUIRE(WildernessReader(data).read(definition) == PARSE_ERROR_NONE);
            const auto expected_level = level.is_null() ? 0 : level.is_object() ? level[selected].get<int>()
                                                                                : level.get<int>();
            const auto &map = std::string_view(map_name) == "normal" ? definition.normal : definition.compact;
            REQUIRE_FALSE(map.letters.empty());
            CHECK(map.letters[0].level == expected_level);
            CHECK(map.letters[0].town == 0);
            CHECK(map.letters[0].road == 0);
            CHECK(map.starting_position == Pos2D(1, 1));
        }
    }
}

/*!
 * @brief 荒野の幅と高さをフロアの大きさの範囲に限ることを確かめる
 */
TEST_CASE("WildernessReader limits dimensions to the floor size")
{
    const auto make_sized = [](int width, int height) {
        auto data = make_definition();
        data["width"] = width;
        data["height"] = height;
        const std::string edge_row(width, '#');
        auto middle_row = edge_row;
        middle_row[1] = '1';
        auto layout = nlohmann::json::array();
        for (auto y = 0; y < height; ++y) {
            layout.push_back(y == 1 ? middle_row : edge_row);
        }
        data["maps"]["normal"]["layout"] = layout;
        return data;
    };
    for (const auto &[width, height] : { std::pair{ 3, 3 }, std::pair{ MAX_WID, MAX_HGT } }) {
        CAPTURE(width);
        CAPTURE(height);
        const auto data = make_sized(width, height);
        WildernessDefinition definition;
        CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_NONE);
        CHECK(definition.width == width);
        CHECK(definition.height == height);
    }
    for (const auto &[width, height] : { std::pair{ 2, 3 }, std::pair{ 3, 2 }, std::pair{ MAX_WID + 1, MAX_HGT }, std::pair{ MAX_WID, MAX_HGT + 1 } }) {
        CAPTURE(width);
        CAPTURE(height);
        const auto data = make_sized(width, height);
        auto definition = make_existing_definition();
        CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_FLAG);
        check_existing_definition(definition);
    }
}

/*!
 * @brief 開始位置を、周囲8マスを生成できる荒野の内側に限ることを確かめる
 */
TEST_CASE("WildernessReader rejects starting positions on the wilderness edge")
{
    for (const auto *map_name : { "normal", "compact" }) {
        CAPTURE(map_name);
        for (const auto *axis : { "x", "y" }) {
            CAPTURE(axis);
            auto data = make_definition();
            data["maps"][map_name]["starting_position"][axis] = 2;
            auto definition = make_existing_definition();
            CHECK(WildernessReader(data).read(definition) == PARSE_ERROR_INVALID_FLAG);
            check_existing_definition(definition);
        }
    }

    // 4×4の荒野では、内側の(2, 2)まで開始位置にできる。
    auto data = make_definition();
    data["width"] = 4;
    data["height"] = 4;
    data["maps"]["normal"]["layout"] = { "####", "#1##", "####", "####" };
    data["maps"]["compact"]["layout"] = { "####", "####", "####", "####" };
    data["maps"]["normal"]["starting_position"] = { { "x", 2 }, { "y", 2 } };
    data["maps"]["compact"]["starting_position"] = { { "x", 2 }, { "y", 2 } };
    WildernessDefinition definition;
    REQUIRE(WildernessReader(data).read(definition) == PARSE_ERROR_NONE);
    CHECK(definition.normal.starting_position == Pos2D(2, 2));
    CHECK(definition.compact.starting_position == Pos2D(2, 2));
}
