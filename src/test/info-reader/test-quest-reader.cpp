/*!
 * @brief 固定クエスト JSONC 読み込みのテスト
 */

#include "artifact/fixed-art-types.h"
#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "info-reader/random-grid-effect-types.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/grid-type-definition.h"
#include "system/terrain/terrain-list.h"
#include "test/system/terrain-list-test-access.h"
#include <doctest/doctest.h>
#include <limits>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
/*!
 * @brief 説明文を1つだけ持つ最小限のクエスト定義を作る
 * @param line 説明文の行 (日本語・英語の両方に使う)
 */
nlohmann::json make_quest_with_description(const std::string &line)
{
    return {
        { "name", { { "ja", "Test" }, { "en", "Test" } } },
        { "definition", { { "type", "KILL_ALL" }, { "level", 5 } } },
        { "descriptions", { { { "text", { { "ja", { line } }, { "en", { line } } } } } } },
    };
}

nlohmann::json make_legend_integer_cell(const std::string &path, const nlohmann::json &value)
{
    auto cell = nlohmann::json::object();
    cell[nlohmann::json::json_pointer(path)] = value;
    if (path.ends_with("/oodLevel") || path.ends_with("/id")) {
        cell[path.substr(1, path.find('/', 1) - 1)]["random"] = true;
    }
    return cell;
}

const std::vector<std::string> QUEST_INTEGER_PATHS = {
    "/definition/level",
    "/definition/numMon",
    "/definition/maxNum",
    "/definition/dungeon",
    "/definition/monster",
    "/definition/reward/artifact",
    "/definition/reward/artifacts/1",
    "/start/y",
    "/start/x",
    "/startVariants/1/y",
    "/startVariants/1/x",
    "/startVariants/1/leavingQuest",
};

bool is_quest_short_id(const std::string &path)
{
    return path == "/definition/monster" || path.starts_with("/definition/reward/");
}

nlohmann::json make_quest_with_integer(const std::string &path, const nlohmann::json &value)
{
    auto data = make_quest_with_description("New description");
    if (path.starts_with("/definition/reward/artifacts/")) {
        data["definition"]["reward"]["artifacts"] = { 1, 2 };
    } else if (path.starts_with("/startVariants/")) {
        data["startVariants"] = { { { "y", 0 }, { "x", 0 } }, { { "y", 1 }, { "x", 2 } } };
    } else if (path.starts_with("/start/")) {
        data["start"] = { { "y", 1 }, { "x", 2 } };
    }
    data[nlohmann::json::json_pointer(path)] = value;
    return data;
}

int get_quest_integer(const QuestFixedMap &fixed_map, const std::string &path)
{
    if (path == "/definition/level") {
        return fixed_map.metadata.level;
    }
    if (path == "/definition/numMon") {
        return fixed_map.metadata.num_mon;
    }
    if (path == "/definition/maxNum") {
        return fixed_map.metadata.max_num;
    }
    if (path == "/definition/dungeon") {
        return fixed_map.metadata.dungeon;
    }
    if (path == "/definition/monster") {
        return fixed_map.metadata.r_idx;
    }
    if (path == "/definition/reward/artifact") {
        return fixed_map.metadata.reward_artifact;
    }
    if (path == "/definition/reward/artifacts/1") {
        return fixed_map.reward_artifact_candidates.at(1);
    }
    const auto &start = fixed_map.starts.at(path.starts_with("/startVariants/") ? 1 : 0);
    if (path.ends_with("/leavingQuest")) {
        return start.leaving_quest.value();
    }
    return path.ends_with("/y") ? start.y : start.x;
}

/*!
 * @brief 読み込み失敗時に保持されるべき既存の出力を用意する
 */
void set_existing_output(QuestType &quest, QuestFixedMap &fixed_map)
{
    quest.name = "Previous quest";
    quest.status = QuestStatusType::TAKEN;
    quest.type = QuestKindType::KILL_NUMBER;
    quest.cur_num = 3;
    quest.level = 99;
    fixed_map.metadata.present = false;
    fixed_map.metadata.type = 3;
    fixed_map.metadata.level = 90;
    fixed_map.metadata.num_mon = 4;
    fixed_map.metadata.max_num = 9;
    fixed_map.metadata.r_idx = 12;
    fixed_map.metadata.dungeon = 7;
    fixed_map.metadata.flags = QUEST_FLAG_ONCE;
    fixed_map.metadata.reward_artifact = 14;
    fixed_map.legend['?'].grid.special = 42;
    fixed_map.maps = { { "previous map" } };
    QuestDescriptionBlock description;
    description.status_equals = QuestStatusType::TAKEN;
    description.lines_ja = { "Previous description ja" };
    description.lines_en = { "Previous description en" };
    fixed_map.descriptions.push_back(std::move(description));
    QuestStartPosition start;
    start.leaving_quest = 77;
    start.y = 5;
    start.x = 4;
    fixed_map.starts.push_back(start);
    fixed_map.reward_artifact_candidates = { 41, 42 };
}

/*!
 * @brief 読み込みエラーが名前・メタデータ・各コレクションを変更していないことを検証する
 */
void check_existing_output(const QuestType &quest, const QuestFixedMap &fixed_map)
{
    CHECK(quest.name == "Previous quest");
    CHECK(quest.status == QuestStatusType::TAKEN);
    CHECK(quest.type == QuestKindType::KILL_NUMBER);
    CHECK(quest.cur_num == 3);
    CHECK(quest.level == 99);
    CHECK_FALSE(fixed_map.metadata.present);
    CHECK(fixed_map.metadata.type == 3);
    CHECK(fixed_map.metadata.level == 90);
    CHECK(fixed_map.metadata.num_mon == 4);
    CHECK(fixed_map.metadata.max_num == 9);
    CHECK(fixed_map.metadata.r_idx == 12);
    CHECK(fixed_map.metadata.dungeon == 7);
    CHECK(fixed_map.metadata.flags == QUEST_FLAG_ONCE);
    CHECK(fixed_map.metadata.reward_artifact == 14);
    REQUIRE(fixed_map.legend.size() == 1);
    CHECK(fixed_map.legend.at('?').grid.special == 42);
    CHECK(fixed_map.maps == std::vector<std::vector<std::string>>{ { "previous map" } });
    REQUIRE(fixed_map.descriptions.size() == 1);
    CHECK(fixed_map.descriptions[0].status_equals == QuestStatusType::TAKEN);
    CHECK(fixed_map.descriptions[0].lines_ja == std::vector<std::string>{ "Previous description ja" });
    CHECK(fixed_map.descriptions[0].lines_en == std::vector<std::string>{ "Previous description en" });
    REQUIRE(fixed_map.starts.size() == 1);
    CHECK(fixed_map.starts[0].leaving_quest == 77);
    CHECK(fixed_map.starts[0].y == 5);
    CHECK(fixed_map.starts[0].x == 4);
    CHECK(fixed_map.reward_artifact_candidates == std::vector<int>{ 41, 42 });
}
}

TEST_CASE("QuestReader reads description lines")
{
    const auto data = make_quest_with_description("Kill them all.");
    QuestType quest;
    QuestFixedMap fixed_map;
    REQUIRE(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_NONE);
    REQUIRE(fixed_map.descriptions.size() == 1);
    CHECK(fixed_map.descriptions[0].lines_ja == std::vector<std::string>{ "Kill them all." });
    CHECK(fixed_map.descriptions[0].lines_en == std::vector<std::string>{ "Kill them all." });
}

TEST_CASE("QuestReader metadata reward and start integers retain representable boundaries")
{
    for (const auto &path : QUEST_INTEGER_PATHS) {
        const auto minimum = is_quest_short_id(path) ? std::numeric_limits<int16_t>::min() : std::numeric_limits<int>::min();
        const auto maximum = is_quest_short_id(path) ? std::numeric_limits<int16_t>::max() : std::numeric_limits<int>::max();
        const std::vector<nlohmann::json> values = { minimum, 0, maximum, static_cast<uint64_t>(maximum) };
        for (const auto &value : values) {
            CAPTURE(path);
            CAPTURE(value);
            const auto data = make_quest_with_integer(path, value);
            QuestType quest;
            QuestFixedMap fixed_map;
            REQUIRE(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_NONE);
            CHECK(get_quest_integer(fixed_map, path) == value.get<int>());
        }
    }
}

TEST_CASE("QuestReader rejects metadata reward and start integer overflow without publishing output")
{
    for (const auto &path : QUEST_INTEGER_PATHS) {
        const int64_t minimum = is_quest_short_id(path) ? std::numeric_limits<int16_t>::min() : std::numeric_limits<int>::min();
        const int64_t maximum = is_quest_short_id(path) ? std::numeric_limits<int16_t>::max() : std::numeric_limits<int>::max();
        const std::vector<nlohmann::json> values = {
            minimum - 1,
            maximum + 1,
            static_cast<uint64_t>(maximum + 1),
            std::numeric_limits<int64_t>::min(),
            std::numeric_limits<int64_t>::max(),
            nlohmann::json::parse("18446744073709551615"),
            nlohmann::json::parse("4294967297"),
        };
        for (const auto &value : values) {
            CAPTURE(path);
            CAPTURE(value);
            const auto data = make_quest_with_integer(path, value);
            QuestType quest;
            QuestFixedMap fixed_map;
            set_existing_output(quest, fixed_map);
            CHECK(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_INVALID_FLAG);
            check_existing_output(quest, fixed_map);
        }
    }
}

TEST_CASE("QuestReader metadata reward and start integers retain null and type behavior")
{
    for (const auto &path : QUEST_INTEGER_PATHS) {
        const auto ignored_non_integer = path.starts_with("/definition/reward/") || path.ends_with("/leavingQuest");
        const auto required = path == "/definition/level" || (path.starts_with("/start") && !path.ends_with("/leavingQuest"));
        const std::vector<nlohmann::json> values = { nullptr, "ignored", 1.5, true, nlohmann::json::array(), nlohmann::json::object() };
        for (const auto &value : values) {
            CAPTURE(path);
            CAPTURE(value);
            const auto data = make_quest_with_integer(path, value);
            QuestType quest;
            QuestFixedMap fixed_map;
            set_existing_output(quest, fixed_map);
            const auto expected_error = ignored_non_integer || (value.is_null() && !required) ? PARSE_ERROR_NONE
                                        : value.is_null()                                     ? PARSE_ERROR_TOO_FEW_ARGUMENTS
                                                                                              : PARSE_ERROR_INVALID_TYPE;
            REQUIRE(QuestReader(data, quest, fixed_map).read() == expected_error);
            if (expected_error != PARSE_ERROR_NONE) {
                check_existing_output(quest, fixed_map);
            } else if (path.ends_with("/leavingQuest")) {
                CHECK_FALSE(fixed_map.starts.at(1).leaving_quest);
            } else if (path == "/definition/reward/artifacts/1") {
                CHECK(fixed_map.reward_artifact_candidates == std::vector<int>{ 1 });
            } else {
                CHECK(get_quest_integer(fixed_map, path) == 0);
            }
        }
    }
}

TEST_CASE("QuestReader start retains precedence over unused start variants")
{
    auto data = make_quest_with_integer("/startVariants/1/leavingQuest", nlohmann::json::parse("18446744073709551615"));
    data["startVariants"][1]["x"] = nlohmann::json::parse("4294967297");
    data["start"] = { { "y", 3 }, { "x", 4 } };
    QuestType quest;
    QuestFixedMap fixed_map;
    REQUIRE(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_NONE);
    REQUIRE(fixed_map.starts.size() == 1);
    CHECK(fixed_map.starts[0].y == 3);
    CHECK(fixed_map.starts[0].x == 4);
    CHECK_FALSE(fixed_map.starts[0].leaving_quest);
}

TEST_CASE("QuestReader preserves previous output on parse errors at every stage")
{
    auto data = make_quest_with_description("New description");
    data["definition"]["flags"] = { "SILENT" };
    data["definition"]["reward"]["artifacts"] = { 1, 2 };
    data["map"] = { "..." };
    data["start"] = { { "y", 0 }, { "x", 0 } };
    int expected_error = PARSE_ERROR_INVALID_TYPE;
    SUBCASE("Invalid root")
    {
        data = nlohmann::json::array();
    }
    SUBCASE("Invalid name")
    {
        data["name"] = 1;
    }
    SUBCASE("Invalid metadata after name")
    {
        data["definition"]["level"] = "wrong type";
    }
    SUBCASE("Invalid descriptions after metadata")
    {
        data["descriptions"] = nlohmann::json::object();
    }
    SUBCASE("Invalid legend after descriptions")
    {
        data["legend"] = { { "too long", nlohmann::json::object() } };
        expected_error = PARSE_ERROR_GENERIC;
    }
    SUBCASE("Invalid map after legend")
    {
        data["map"] = nlohmann::json::object();
    }
    SUBCASE("Invalid start after map")
    {
        data["start"].erase("x");
        expected_error = PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    SUBCASE("Invalid later map variant")
    {
        data.erase("map");
        data["mapVariants"] = { { "..." }, nlohmann::json::object() };
    }
    SUBCASE("Invalid later start variant")
    {
        data.erase("start");
        data["startVariants"] = { { { "y", 0 }, { "x", 0 } }, { { "y", 1 } } };
        expected_error = PARSE_ERROR_TOO_FEW_ARGUMENTS;
    }
    QuestType quest;
    QuestFixedMap fixed_map;
    set_existing_output(quest, fixed_map);
    CHECK(QuestReader(data, quest, fixed_map).read() == expected_error);
    check_existing_output(quest, fixed_map);
}

TEST_CASE("QuestReader rejects non-string flags without changing previous output")
{
    auto data = make_quest_with_description("New description");
    SUBCASE("Definition flag")
    {
        data["definition"]["flags"] = { "SILENT", 1 };
    }
    SUBCASE("Legend caveInfo flag")
    {
        data["definition"]["flags"] = { "SILENT" };
        data["legend"] = { { ".", { { "caveInfo", { "GLOW", 1 } } } } };
    }
    QuestType quest;
    QuestFixedMap fixed_map;
    set_existing_output(quest, fixed_map);
    CHECK(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_INVALID_TYPE);
    check_existing_output(quest, fixed_map);
}

TEST_CASE("QuestReader copies valid caveInfo flags into legend grids")
{
    // 凡例の解析は地形IDを格納するだけで、地形要素を参照しない。
    test::TerrainListTestAccess terrain_tag(TerrainTag::NONE, 0);
    auto data = make_quest_with_description("A lit room");
    data["legend"] = {
        { ".", { { "caveInfo", { "GLOW", "ROOM" } } } },
        { "#", { { "caveInfo", { "MARK" } } } },
    };
    data["map"] = { ".#" };

    QuestType quest;
    QuestFixedMap fixed_map;
    set_existing_output(quest, fixed_map);
    REQUIRE(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_NONE);
    REQUIRE(fixed_map.legend.size() == 2);
    CHECK(fixed_map.legend.at('.').grid.cave_info == (CAVE_GLOW | CAVE_ROOM));
    CHECK(fixed_map.legend.at('#').grid.cave_info == CAVE_MARK);
    CHECK(fixed_map.legend.count('?') == 0);
    CHECK(fixed_map.maps == std::vector<std::vector<std::string>>{ { ".#" } });
}

TEST_CASE("TerrainList test tag access restores tags after scope and exception")
{
    auto &terrains = TerrainList::get_instance();
    const auto original_tags = test::TerrainListTestAccess::current_tags();
    {
        test::TerrainListTestAccess terrain_tag(TerrainTag::NONE, 0);
        CHECK(terrains.get_terrain_id(TerrainTag::NONE) == 0);
    }
    CHECK(test::TerrainListTestAccess::current_tags() == original_tags);

    const auto throw_with_tag = [] {
        test::TerrainListTestAccess terrain_tag(TerrainTag::NONE, 0);
        throw std::runtime_error("test exception");
    };
    CHECK_THROWS_AS(throw_with_tag(), std::runtime_error);
    CHECK(test::TerrainListTestAccess::current_tags() == original_tags);
}

TEST_CASE("Quest legend integers retain representable signed and unsigned boundaries")
{
    test::TerrainListTestAccess terrain_tag(TerrainTag::NONE, 0);
    const std::vector<std::string> paths = {
        "/monster",
        "/monster/cloneOf",
        "/monster/oodLevel",
        "/object",
        "/object/oodLevel",
        "/ego",
        "/ego/id",
        "/artifact",
        "/artifact/id",
        "/special",
    };
    for (const auto &path : paths) {
        const auto is_ego = path.starts_with("/ego");
        const auto is_monster_id = path == "/monster" || path == "/monster/cloneOf";
        const auto maximum = is_ego ? std::numeric_limits<int>::max() : std::numeric_limits<int16_t>::max();
        const auto minimum = is_ego ? std::numeric_limits<int>::min() : is_monster_id ? -maximum
                                                                                      : std::numeric_limits<int16_t>::min();
        const std::vector<nlohmann::json> values = { minimum, 0, static_cast<uint64_t>(maximum) };
        for (const auto &value : values) {
            CAPTURE(path);
            CAPTURE(value);
            const auto cell_data = make_legend_integer_cell(path, value);
            QuestLegendCell cell;
            REQUIRE(parse_quest_legend_cell(cell_data, cell) == PARSE_ERROR_NONE);
            const auto expected_random = path.ends_with("/oodLevel") ? (path.starts_with("/monster") ? RANDOM_MONSTER : RANDOM_OBJECT)
                                         : path.ends_with("/id")     ? (is_ego ? RANDOM_EGO : RANDOM_ARTIFACT)
                                                                     : RANDOM_NONE;
            CHECK(cell.grid.random == expected_random);
            const auto expected = value.get<int>();
            if (path.starts_with("/monster")) {
                CHECK(cell.grid.monster == (path == "/monster/cloneOf" ? -expected : expected));
            } else if (path.starts_with("/object")) {
                CHECK(cell.grid.object == expected);
            } else if (is_ego) {
                CHECK(static_cast<int>(cell.grid.ego) == expected);
            } else if (path.starts_with("/artifact")) {
                CHECK(static_cast<int>(cell.grid.artifact) == expected);
            } else {
                CHECK(cell.grid.special == expected);
            }
        }
    }
}

TEST_CASE("QuestReader rejects legend integer overflow without publishing output")
{
    test::TerrainListTestAccess terrain_tag(TerrainTag::NONE, 0);
    const std::vector<std::string> paths = {
        "/monster",
        "/monster/cloneOf",
        "/monster/oodLevel",
        "/object",
        "/object/oodLevel",
        "/ego",
        "/ego/id",
        "/artifact",
        "/artifact/id",
        "/special",
    };
    for (const auto &path : paths) {
        const auto is_ego = path.starts_with("/ego");
        const auto is_monster_id = path == "/monster" || path == "/monster/cloneOf";
        const int64_t maximum = is_ego ? std::numeric_limits<int>::max() : std::numeric_limits<int16_t>::max();
        const int64_t minimum = is_ego ? std::numeric_limits<int>::min() : is_monster_id ? -maximum
                                                                                         : std::numeric_limits<int16_t>::min();
        std::vector<nlohmann::json> values = {
            minimum - 1,
            maximum + 1,
            static_cast<uint64_t>(maximum + 1),
            std::numeric_limits<int64_t>::min(),
            std::numeric_limits<int64_t>::max(),
            std::numeric_limits<uint64_t>::max(),
            uint64_t{ 4294967297 },
        };
        if (!is_ego) {
            values.emplace_back(std::numeric_limits<int>::min());
        }
        for (const auto &value : values) {
            CAPTURE(path);
            CAPTURE(value);
            auto data = make_quest_with_description("New description");
            data["legend"] = { { ".", make_legend_integer_cell(path, value) } };
            QuestType quest;
            QuestFixedMap fixed_map;
            set_existing_output(quest, fixed_map);
            CHECK(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_INVALID_FLAG);
            check_existing_output(quest, fixed_map);
        }
    }
}

TEST_CASE("Quest legend preserves optional non-integer defaults and reward markers")
{
    test::TerrainListTestAccess terrain_tag(TerrainTag::NONE, 0);
    const std::vector<std::string> paths = {
        "/monster",
        "/monster/cloneOf",
        "/monster/oodLevel",
        "/object",
        "/object/oodLevel",
        "/ego",
        "/ego/id",
        "/artifact",
        "/artifact/id",
    };
    for (const auto &path : paths) {
        const std::vector<nlohmann::json> values = { nullptr, "ignored", 1.5, true };
        for (const auto &value : values) {
            CAPTURE(path);
            CAPTURE(value);
            const auto data = make_legend_integer_cell(path, value);
            QuestLegendCell cell;
            REQUIRE(parse_quest_legend_cell(data, cell) == PARSE_ERROR_NONE);
            CHECK(cell.grid.monster == 0);
            CHECK(cell.grid.object == 0);
            CHECK(cell.grid.ego == EgoType::NONE);
            CHECK(cell.grid.artifact == FixedArtifactId::NONE);
        }
    }
    const nlohmann::json rewards = {
        { "object", { { "questReward", true } } },
        { "artifact", { { "questReward", true } } },
    };
    QuestLegendCell cell;
    REQUIRE(parse_quest_legend_cell(rewards, cell) == PARSE_ERROR_NONE);
    CHECK(cell.object_is_quest_reward);
    CHECK(cell.artifact_is_quest_reward);
}

TEST_CASE("Quest legend random forms preserve precedence over clone and reward markers")
{
    test::TerrainListTestAccess terrain_tag(TerrainTag::NONE, 0);
    const nlohmann::json data = {
        { "monster", { { "random", true }, { "oodLevel", 7 }, { "cloneOf", std::numeric_limits<uint64_t>::max() } } },
        { "object", { { "random", true }, { "oodLevel", 8 }, { "questReward", true } } },
        { "ego", { { "random", true }, { "id", 9 } } },
        { "artifact", { { "random", true }, { "id", 10 }, { "questReward", true } } },
    };
    QuestLegendCell cell;
    REQUIRE(parse_quest_legend_cell(data, cell) == PARSE_ERROR_NONE);
    CHECK(cell.grid.random == (RANDOM_MONSTER | RANDOM_OBJECT | RANDOM_EGO | RANDOM_ARTIFACT));
    CHECK(cell.grid.monster == 7);
    CHECK(cell.grid.object == 8);
    CHECK(static_cast<int>(cell.grid.ego) == 9);
    CHECK(static_cast<int>(cell.grid.artifact) == 10);
    CHECK_FALSE(cell.object_is_quest_reward);
    CHECK_FALSE(cell.artifact_is_quest_reward);
}

TEST_CASE("Quest legend special retains optional and invalid-type behavior")
{
    test::TerrainListTestAccess terrain_tag(TerrainTag::NONE, 0);
    const nlohmann::json null_special = { { "special", nullptr } };
    QuestLegendCell cell;
    REQUIRE(parse_quest_legend_cell(null_special, cell) == PARSE_ERROR_NONE);
    CHECK(cell.grid.special == 0);
    const nlohmann::json invalid_special = { { "special", "invalid" } };
    CHECK(parse_quest_legend_cell(invalid_special, cell) == PARSE_ERROR_INVALID_TYPE);
}

TEST_CASE("QuestReader replaces output on success without duplicating collections or resetting quest progress")
{
    auto data = make_quest_with_description("New description");
    data["definition"]["flags"] = { "SILENT" };
    data["definition"]["reward"]["artifacts"] = { 1, 2 };
    data["map"] = { "..." };
    data["start"] = { { "y", 0 }, { "x", 0 } };
    QuestType quest;
    QuestFixedMap fixed_map;
    set_existing_output(quest, fixed_map);
    const QuestReader reader(data, quest, fixed_map);
    for (int attempt = 0; attempt < 2; ++attempt) {
        CAPTURE(attempt);
        REQUIRE(reader.read() == PARSE_ERROR_NONE);
        CHECK(quest.name == "Test");
        CHECK(quest.status == QuestStatusType::TAKEN);
        CHECK(quest.type == QuestKindType::KILL_NUMBER);
        CHECK(quest.cur_num == 3);
        CHECK(quest.level == 99);
        CHECK(fixed_map.metadata.present);
        CHECK(fixed_map.metadata.type == static_cast<int>(QuestKindType::KILL_ALL));
        CHECK(fixed_map.metadata.level == 5);
        CHECK(fixed_map.metadata.flags == QUEST_FLAG_SILENT);
        CHECK(fixed_map.metadata.reward_artifact == 0);
        CHECK(fixed_map.legend.empty());
        CHECK(fixed_map.maps == std::vector<std::vector<std::string>>{ { "..." } });
        REQUIRE(fixed_map.descriptions.size() == 1);
        CHECK(fixed_map.descriptions[0].lines_ja == std::vector<std::string>{ "New description" });
        CHECK(fixed_map.descriptions[0].lines_en == std::vector<std::string>{ "New description" });
        REQUIRE(fixed_map.starts.size() == 1);
        CHECK(fixed_map.starts[0].y == 0);
        CHECK(fixed_map.starts[0].x == 0);
        CHECK_FALSE(fixed_map.starts[0].leaving_quest.has_value());
        CHECK(fixed_map.reward_artifact_candidates == std::vector<int>{ 1, 2 });
    }

    data = make_quest_with_description("Another quest");
    data["name"] = { { "ja", "Another quest" }, { "en", "Another quest" } };
    data["definition"]["level"] = 17;
    data["definition"]["reward"]["artifact"] = 7;
    data.erase("descriptions");
    REQUIRE(reader.read() == PARSE_ERROR_NONE);
    CHECK(quest.name == "Another quest");
    CHECK(quest.status == QuestStatusType::TAKEN);
    CHECK(quest.type == QuestKindType::KILL_NUMBER);
    CHECK(quest.cur_num == 3);
    CHECK(quest.level == 99);
    CHECK(fixed_map.metadata.present);
    CHECK(fixed_map.metadata.type == static_cast<int>(QuestKindType::KILL_ALL));
    CHECK(fixed_map.metadata.level == 17);
    CHECK(fixed_map.metadata.reward_artifact == 7);
    CHECK(fixed_map.metadata.num_mon == 0);
    CHECK(fixed_map.metadata.max_num == 0);
    CHECK(fixed_map.metadata.r_idx == 0);
    CHECK(fixed_map.metadata.dungeon == 0);
    CHECK(fixed_map.metadata.flags == 0);
    CHECK(fixed_map.legend.empty());
    CHECK(fixed_map.maps.empty());
    CHECK(fixed_map.descriptions.empty());
    CHECK(fixed_map.starts.empty());
    CHECK(fixed_map.reward_artifact_candidates.empty());
}

#ifdef JP
TEST_CASE("QuestReader rejects a description line which cannot be converted to the system encoding")
{
    // 日本語版では説明文をシステムの文字コードに変換するので、変換できない行 (途中に '\0' を含むなど) は不正とする
    const auto data = make_quest_with_description(std::string("Kill\0them", 9));
    QuestType quest;
    QuestFixedMap fixed_map;
    set_existing_output(quest, fixed_map);
    CHECK(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_INVALID_VALUE);
    check_existing_output(quest, fixed_map);
}
#endif
