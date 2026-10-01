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

#ifdef JP
TEST_CASE("QuestReader rejects a description line which cannot be converted to the system encoding")
{
    // 日本語版では説明文をシステムの文字コードに変換するので、変換できない行 (途中に '\0' を含むなど) は不正とする
    const auto data = make_quest_with_description(std::string("Kill\0them", 9));
    QuestType quest;
    QuestFixedMap fixed_map;
    CHECK(QuestReader(data, quest, fixed_map).read() == PARSE_ERROR_INVALID_VALUE);
}
#endif
