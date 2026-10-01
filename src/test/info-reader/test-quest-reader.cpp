/*!
 * @brief 固定クエスト JSONC 読み込みのテスト
 */

#include "info-reader/parse-error-types.h"
#include "info-reader/quest-reader.h"
#include "system/dungeon/quest-definition.h"
#include "system/dungeon/quest-fixed-map.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
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

TEST_CASE("QuestReader preserves previous output when a JSON type conversion throws")
{
    auto data = make_quest_with_description("New description");
    data["definition"]["flags"] = { "SILENT", 1 };
    QuestType quest;
    QuestFixedMap fixed_map;
    set_existing_output(quest, fixed_map);
    CHECK_THROWS_AS(QuestReader(data, quest, fixed_map).read(), nlohmann::json::type_error);
    check_existing_output(quest, fixed_map);
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
