/*!
 * @brief レコード共通診断のキー検証と整数検証のテスト
 */
#include "info-reader/record-diagnostic.h"
#include "locale/language-switcher.h"
#include <array>
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

TEST_CASE("RecordDiagnostic checks required keys before unknown keys")
{
    DefinitionIdChecker session(DefinitionIdOrder::SEQUENTIAL, { 0, 9 }, "record");
    RecordDiagnostic reader(session);
    const auto keys = std::to_array<std::string_view>({ "id", "value" });
    nlohmann::json data = { { "id", 3 }, { "typo", 1 } };
    auto expected = ParseErrorType::TOO_FEW_ARGUMENTS;
    std::string path = "$.value";
    std::string reason = _("必須項目がありません", "missing required field");
    SUBCASE("unknown key")
    {
        data["value"] = 1;
        expected = ParseErrorType::UNDEFINED_DIRECTIVE;
        path = "$.typo";
        reason = _("未知の項目です", "unknown field");
    }
    SUBCASE("not an object")
    {
        data = nullptr;
        expected = ParseErrorType::INVALID_TYPE;
        path = "$";
        reason = _("オブジェクトが必要です", "expected an object");
    }
    reader.begin_read(data);
    const auto result = reader.check_keys(data, "$", keys);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == expected);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->path == path);
    CHECK(reader.error()->reason == reason);
    CHECK(reader.error()->id == (data.is_object() ? "3" : "<unknown>"));
}

TEST_CASE("RecordDiagnostic delegates only a missing root id to the session")
{
    DefinitionIdChecker session(DefinitionIdOrder::SEQUENTIAL, { 0, 9 }, "record");
    RecordDiagnostic reader(session);
    const nlohmann::json data = nlohmann::json::object();
    const auto keys = std::to_array<std::string_view>({ "id" });
    std::string path = "$";
    std::string reason = fmt::format(_("{}の ID がありません", "Missing {} ID"), "record");
    SUBCASE("nested id is an ordinary required field")
    {
        path = "$.child";
        reason = _("必須項目がありません", "missing required field");
    }
    reader.begin_read(data);
    const auto result = reader.check_keys(data, path, keys);
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == ParseErrorType::TOO_FEW_ARGUMENTS);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->id == "<unknown>");
    CHECK(reader.error()->path == path + ".id");
    CHECK(reader.error()->reason == reason);
    CHECK_FALSE(session.last_id().has_value());
}

TEST_CASE("RecordDiagnostic preserves integer codes and distinguishes type and range reasons")
{
    DefinitionIdChecker session(DefinitionIdOrder::SEQUENTIAL, { 0, 9 }, "record");
    nlohmann::json data = "2";
    auto expected = ParseErrorType::INVALID_TYPE;
    std::string reason = _("整数が必要です", "expected an integer");
    SUBCASE("out of range")
    {
        data = 5;
        expected = ParseErrorType::INVALID_VALUE;
        reason = _("0以上4以下の整数が必要です", "expected an integer in [0, 4]");
    }
    SUBCASE("floating point value")
    {
        data = 2.5;
    }
    SUBCASE("required null")
    {
        data = nullptr;
        expected = ParseErrorType::TOO_FEW_ARGUMENTS;
    }
    RecordDiagnostic reader(session);
    reader.begin_read(nlohmann::json::object());
    int value = 99;
    const auto result = reader.read_integer(data, value, 0, 4, "$.value");
    REQUIRE_FALSE(result.has_value());
    CHECK(result.error() == expected);
    REQUIRE(reader.error().has_value());
    CHECK(reader.error()->path == "$.value");
    CHECK(reader.error()->reason == reason);
    CHECK(value == 99);
}

TEST_CASE("RecordDiagnostic accepts valid keys and inclusive integer bounds and resets errors")
{
    DefinitionIdChecker session(DefinitionIdOrder::SEQUENTIAL, { 0, 9 }, "record");
    RecordDiagnostic reader(session);
    nlohmann::json data = { { "id", "bad" } };
    reader.begin_read(data);
    CHECK_FALSE(reader.fail(ParseErrorType::INVALID_VALUE, "$.id", "test").has_value());
    REQUIRE(reader.error().has_value());
    data["id"] = 2;
    reader.begin_read(data);
    CHECK_FALSE(reader.error().has_value());
    const auto keys = std::to_array<std::string_view>({ "id" });
    CHECK(reader.check_keys(data, "$", keys).has_value());
    CHECK_FALSE(session.last_id().has_value());
    for (const int bound : { 1, 4 }) {
        int value = 0;
        CHECK(reader.read_integer(nlohmann::json(bound), value, 1, 4, "$.value").has_value());
        CHECK(value == bound);
        CHECK_FALSE(reader.error().has_value());
    }
}
