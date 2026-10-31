#include "info-reader/legend-object-trap-validator.h"
#include "info-reader/quest-reader.h"
#include "info-reader/town-preferences-reader.h"
#include "locale/language-switcher.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/info-reader/legend-ego-fixture.h"
#include "test/info-reader/parse-result-helpers.h"
#include "test/info-reader/scoped-reader-state.h"
#include "test/scoped-vector-wrapper.h"
#include "test/system/terrain-list-test-access.h"
#include "view/display-messages.h"
#include <cstdint>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>

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

tl::expected<void, ParseErrorType> parse_test_cell(const nlohmann::json &data, QuestLegendCell &cell, std::string &)
{
    const auto &terrain = data["terrain"].get_ref<const std::string &>();
    if (terrain == "INVALID") {
        return tl::make_unexpected(ParseErrorType::UNDEFINED_TERRAIN_TAG);
    }
    for (const auto &flag : data["caveInfo"]) {
        if (flag == "INVALID_FLAG") {
            return tl::make_unexpected(ParseErrorType::INVALID_FLAG);
        }
    }
    cell.grid.feature = terrain == "FLOOR" ? 10 : 20;
    cell.grid.cave_info = static_cast<BIT_FLAGS>(data["caveInfo"].size());
    cell.grid.special = static_cast<int16_t>(data.value("special", 0));
    return {};
}

void check_rejected(const nlohmann::json &data, ParseErrorType expected)
{
    TownPreferencesLegend legend{ { '?', dungeon_grid{} } };
    legend.front().second.feature = 99;
    CHECK(test::to_error_code(TownPreferencesReader(data).read(legend, parse_test_cell)) == expected);
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
    CHECK(TownPreferencesReader(data).read(legend, parse_test_cell).has_value());
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
        check_rejected(root, root.is_object() ? ParseErrorType::TOO_FEW_ARGUMENTS : ParseErrorType::INVALID_TYPE);
    }
    for (const auto &version : { nlohmann::json(0), nlohmann::json(1.0), nlohmann::json("1") }) {
        auto data = make_town_preferences();
        data["version"] = version;
        check_rejected(data, version.is_number_integer() ? ParseErrorType::INVALID_VALUE : ParseErrorType::INVALID_TYPE);
    }
    for (const auto &legend : { nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json::object() }) {
        auto data = make_town_preferences();
        data["legend"] = legend;
        check_rejected(data, ParseErrorType::INVALID_TYPE);
    }
}

TEST_CASE("TownPreferencesReader validates printable one-byte symbols")
{
    for (const auto *symbol : { "", "AB", ":", "/", "\\", "\xC3\xA9" }) {
        auto data = make_town_preferences();
        data["legend"][symbol] = data["legend"]["A"];
        check_rejected(data, ParseErrorType::INVALID_VALUE);
    }
    for (const auto *symbol : { " ", "!", "~" }) {
        auto data = make_town_preferences();
        data["legend"] = { { symbol, { { "terrain", "FLOOR" }, { "caveInfo", nlohmann::json::array() } } } };
        TownPreferencesLegend legend;
        CHECK(TownPreferencesReader(data).read(legend, parse_test_cell).has_value());
        REQUIRE(legend.size() == 1);
        CHECK(legend.front().first == symbol[0]);
    }
}

TEST_CASE("TownPreferencesReader validates cell shape and cave flags")
{
    for (const auto &cell : { nlohmann::json(nullptr), nlohmann::json::array(), nlohmann::json(1) }) {
        auto data = make_town_preferences();
        data["legend"]["b"] = cell;
        check_rejected(data, ParseErrorType::INVALID_TYPE);
    }
    for (const auto &terrain : { nlohmann::json(nullptr), nlohmann::json(1) }) {
        auto data = make_town_preferences();
        data["legend"]["b"]["terrain"] = terrain;
        check_rejected(data, ParseErrorType::INVALID_TYPE);
    }
    for (const auto &cave_info : { nlohmann::json(nullptr), nlohmann::json("MARK"), nlohmann::json::object() }) {
        auto data = make_town_preferences();
        data["legend"]["b"]["caveInfo"] = cave_info;
        check_rejected(data, ParseErrorType::INVALID_TYPE);
    }
    auto data = make_town_preferences();
    data["legend"]["b"].erase("terrain");
    check_rejected(data, ParseErrorType::INVALID_TYPE);
    data = make_town_preferences();
    data["legend"]["b"].erase("caveInfo");
    check_rejected(data, ParseErrorType::INVALID_TYPE);
    data = make_town_preferences();
    data["legend"]["b"]["caveInfo"] = { "MARK", 2 };
    check_rejected(data, ParseErrorType::INVALID_TYPE);
}

TEST_CASE("TownPreferencesReader validates special within signed 16-bit range")
{
    for (const auto &special : { nlohmann::json(nullptr), nlohmann::json("1"), nlohmann::json(1.0), nlohmann::json(false), nlohmann::json::array(), nlohmann::json::object(),
             nlohmann::json(-32769), nlohmann::json(32768), nlohmann::json(std::numeric_limits<int64_t>::min()), nlohmann::json(std::numeric_limits<int64_t>::max()), nlohmann::json(std::numeric_limits<uint64_t>::max()) }) {
        CAPTURE(special);
        auto data = make_town_preferences();
        data["legend"]["b"]["special"] = special;
        check_rejected(data, ParseErrorType::INVALID_VALUE);
    }
    for (const auto special : { -32768, 32767 }) {
        auto data = make_town_preferences();
        data["legend"]["b"]["special"] = special;
        TownPreferencesLegend legend;
        CHECK(TownPreferencesReader(data).read(legend, parse_test_cell).has_value());
        REQUIRE(legend.size() == 2);
        CHECK(legend[1].second.special == special);
    }
}

TEST_CASE("TownPreferencesReader preserves unsigned special boundaries after JSON parsing")
{
    for (const int special : { 0, 32767, 32768 }) {
        CAPTURE(special);
        auto input = make_town_preferences();
        input["legend"]["b"]["special"] = special;
        const auto data = nlohmann::json::parse(input.dump());
        REQUIRE(data["legend"]["b"]["special"].is_number_unsigned());
        if (special == 32768) {
            check_rejected(data, ParseErrorType::INVALID_VALUE);
            continue;
        }
        TownPreferencesLegend legend;
        REQUIRE(TownPreferencesReader(data).read(legend, parse_test_cell).has_value());
        REQUIRE(legend.size() == 2);
        CHECK(legend[1].second.special == special);
    }
}

TEST_CASE("TownPreferencesReader propagates cell parser errors without partial output")
{
    auto data = make_town_preferences();
    data["legend"]["b"]["terrain"] = "INVALID";
    check_rejected(data, ParseErrorType::UNDEFINED_TERRAIN_TAG);
    data = make_town_preferences();
    data["legend"]["b"]["caveInfo"] = { "INVALID_FLAG" };
    check_rejected(data, ParseErrorType::INVALID_FLAG);
}

TEST_CASE("TownPreferencesReader retains permissive runtime behavior for extra fields")
{
    auto data = make_town_preferences();
    data["unused"] = true;
    data["legend"]["A"]["unused"] = true;
    TownPreferencesLegend legend;
    CHECK(TownPreferencesReader(data).read(legend, parse_test_cell).has_value());
    CHECK(legend.size() == 2);
}

TEST_CASE("TownPreferencesReader reports incompatible ego with legend key")
{
    const test::ScopedReaderState reader_state;
    const test::LegendEgoFixture ego_state;
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    auto data = make_town_preferences();
    data["legend"]["A"]["terrain"] = "*";
    data["legend"]["A"]["object"] = 54;
    data["legend"]["A"]["ego"] = static_cast<int>(EgoType::DF);
    ScopedMessageCapture diagnostics;
    TownPreferencesLegend legend;
    CHECK(test::to_error_code(TownPreferencesReader(data).read(legend, parse_quest_legend_cell)) == ParseErrorType::INVALID_VALUE);
    CHECK(legend.empty());
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages().front() == _("町のエゴ指定が不正です: legend['A'].ego 65 (object 54): 品の種類と相性が合わない",
                                                "Invalid town ego: legend['A'].ego 65 (object 54): incompatible object kind"));
}

TEST_CASE("TownPreferencesReader names a random object in ego diagnostics")
{
    const test::ScopedReaderState reader_state;
    const test::LegendEgoFixture ego_state;
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    auto data = make_town_preferences();
    data["legend"]["A"]["terrain"] = "*";
    data["legend"]["A"]["object"] = nlohmann::json{ { "random", true }, { "oodLevel", 12 } };
    data["legend"]["A"]["ego"] = static_cast<int>(EgoType::DF);
    ScopedMessageCapture diagnostics;
    TownPreferencesLegend legend;
    CHECK(test::to_error_code(TownPreferencesReader(data).read(legend, parse_quest_legend_cell)) == ParseErrorType::INVALID_VALUE);
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages().front() == _("町のエゴ指定が不正です: legend['A'].ego 65 (object random): ランダムな品には指定できない",
                                                "Invalid town ego: legend['A'].ego 65 (object random): cannot apply to a random object"));
}

TEST_CASE("TownPreferencesReader reports object trap conflicts with legend key and fixed trap tag")
{
    const test::ScopedReaderState reader_state;
    const test::TerrainListTestAccess none_tag(TerrainTag::NONE, 0);
    auto &terrains = TerrainList::get_instance();
    const test::ScopedVectorWrapper<TerrainList> terrain_state(terrains);
    terrains.resize(3);
    terrains.get_terrain(0).tag = "NONE";
    terrains.get_terrain(1).tag = "TRAP_PIT";
    terrains.get_terrain(1).flags.set(TerrainCharacteristics::TRAP);
    terrains.get_terrain(2).tag = "FLOOR";
    for (const auto *tag : { "TRAP_PIT", "FLOOR" }) {
        CAPTURE(tag);
        auto data = make_town_preferences();
        data["legend"]["A"]["terrain"] = "*";
        data["legend"]["A"]["object"] = 54;
        data["legend"]["A"]["trap"] = tag;
        ScopedMessageCapture diagnostics;
        TownPreferencesLegend legend;
        CHECK(test::to_error_code(TownPreferencesReader(data).read(legend, parse_quest_legend_cell)) == ParseErrorType::INVALID_VALUE);
        CHECK(legend.empty());
        REQUIRE(diagnostics.messages().size() == 1);
        // 町のマップの featureRules と同じく、罠の地形のタグだけは未知のタグや誤記への言及を付けない。
        const auto known = std::string_view(tag) == "TRAP_PIT";
        const auto hint = known ? std::string(_("固定の罠として扱われます", "is treated as a fixed trap"))
                                : std::string(_("固定の罠として扱われます。未知のタグや誤記も固定の罠になります", "is treated as a fixed trap; unknown tags and typos are treated the same way"));
        const auto expected = fmt::format(_("町の配置指定が不正です: legend['A'].trap: {} (trap '{}' は{})", "Invalid town placement: legend['A'].trap: {} (trap '{}' {})"),
            describe_legend_object_trap_conflict(), tag, hint);
        CHECK(diagnostics.messages().front() == expected);
    }
}

TEST_CASE("TownPreferencesReader adds the fixed trap tag note to conflicts found after the cell parser")
{
    const test::ScopedReaderState reader_state;
    auto &terrains = TerrainList::get_instance();
    const test::ScopedVectorWrapper<TerrainList> terrain_state(terrains);
    terrains.resize(2);
    terrains.get_terrain(0).tag = "NONE";
    terrains.get_terrain(1).tag = "TRAP_PIT";
    terrains.get_terrain(1).flags.set(TerrainCharacteristics::TRAP);
    const auto data = nlohmann::json::parse(R"({"version":1,"legend":{"#":{"terrain":"NONE","caveInfo":[],"trap":"TRAP_PIT"}}})");
    // 衝突を拒否しない解析器を差し込み、読み込み側の衝突の検査が出す表示を確かめる。
    const auto parse_cell = +[](const nlohmann::json &, QuestLegendCell &cell, std::string &) -> tl::expected<void, ParseErrorType> {
        cell.grid.object = 54;
        cell.grid.trap = 1;
        return {};
    };
    ScopedMessageCapture diagnostics;
    TownPreferencesLegend legend;
    CHECK(test::to_error_code(TownPreferencesReader(data).read(legend, parse_cell)) == ParseErrorType::INVALID_VALUE);
    CHECK(legend.empty());
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages().front() == _("町の配置指定が不正です: legend['#'].trap: 品と罠の指定の片方が配置時に無視される組み合わせです (trap 'TRAP_PIT' は固定の罠として扱われます)",
                                                "Invalid town placement: legend['#'].trap: This object/trap combination silently ignores one placement (trap 'TRAP_PIT' is treated as a fixed trap)"));
}

TEST_CASE("TownPreferencesReader reports reasons from an injected cell parser")
{
    const auto data = nlohmann::json::parse(R"({"version":1,"legend":{"#":{"terrain":"NONE","caveInfo":[]}}})");
    TownPreferencesLegend legend{ { '?', dungeon_grid{} } };
    const auto parse_cell = +[](const nlohmann::json &, QuestLegendCell &, std::string &reason) -> tl::expected<void, ParseErrorType> {
        reason = "Injected reason";
        return tl::make_unexpected(ParseErrorType::INVALID_VALUE);
    };
    ScopedMessageCapture diagnostics;
    CHECK(test::to_error_code(TownPreferencesReader(data).read(legend, parse_cell)) == ParseErrorType::INVALID_VALUE);
    REQUIRE(legend.size() == 1);
    CHECK(legend.front().first == '?');
    REQUIRE(diagnostics.messages().size() == 1);
    CHECK(diagnostics.messages().front() == _("町の配置指定が不正です: legend['#'].trap: Injected reason",
                                                "Invalid town placement: legend['#'].trap: Injected reason"));
}

TEST_CASE("TownPreferencesReader classifies version errors before legend errors")
{
    auto data = make_town_preferences();
    data.erase("version");
    check_rejected(data, ParseErrorType::TOO_FEW_ARGUMENTS);
    data["version"] = nullptr;
    check_rejected(data, ParseErrorType::TOO_FEW_ARGUMENTS);
    data["version"] = 2;
    check_rejected(data, ParseErrorType::INVALID_VALUE);
    for (const auto &version : { nlohmann::json(1.0), nlohmann::json("1"), nlohmann::json(true) }) {
        data["version"] = version;
        check_rejected(data, ParseErrorType::INVALID_TYPE);
    }
}
