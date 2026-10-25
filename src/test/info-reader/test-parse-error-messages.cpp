/*!
 * @brief ゲームデータ解析エラー名の表のテスト
 */

#include "info-reader/parse-error-messages.h"
#include "info-reader/parse-error-types.h"
#include "locale/character-encoding.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace {
std::string describe(const nlohmann::json &record)
{
    return describe_record_identifier(find_record_identifier(record));
}
}

TEST_CASE("format_definition_load_error preserves diagnostics before the record error")
{
    const auto record = format_record_parse_error("Definitions.jsonc", "records", 3, "", ParseErrorType::INVALID_FLAG);
    CHECK(format_definition_load_error({}, record) == record);
    const std::vector<std::string> diagnostics{ "Unknown flag", "", "Failed to load flags" };
    CHECK(format_definition_load_error(diagnostics, record) == "Unknown flag\nFailed to load flags\n" + record);

    // システムの文字コードのバイト列を変換せず、そのまま残す。
    const std::vector<std::string> encoded{ std::string("\x82\xa0\xa4\xa2") };
    CHECK(format_definition_load_error(encoded, record) == encoded.front() + "\n" + record);
}

TEST_CASE("describe_parse_error names every parse error")
{
    // ParseErrorType::NONE (0) はエラーではないので名称を持たず、代替名になる
    const auto fallback = describe_parse_error(ParseErrorType::NONE);
    for (auto code = enum2i(ParseErrorType::GENERIC); code < enum2i(ParseErrorType::MAX); ++code) {
        CAPTURE(code);
        const auto err = static_cast<ParseErrorType>(code);
        CHECK_FALSE(describe_parse_error(err).empty());
        CHECK(describe_parse_error(err) != fallback);
    }
}

TEST_CASE("describe_parse_error falls back for non-errors and out of range values")
{
    const auto fallback = describe_parse_error(ParseErrorType::NONE);
    CHECK_FALSE(fallback.empty());
    CHECK(describe_parse_error(ParseErrorType::MAX) == fallback);
    CHECK(describe_parse_error(static_cast<ParseErrorType>(-1)) == fallback);
    CHECK(describe_parse_error(static_cast<ParseErrorType>(enum2i(ParseErrorType::MAX) + 1)) == fallback);
}

TEST_CASE("format_record_parse_error includes the file position name and code")
{
    for (auto code = enum2i(ParseErrorType::GENERIC); code < enum2i(ParseErrorType::MAX); ++code) {
        CAPTURE(code);
        const auto err = static_cast<ParseErrorType>(code);
        const auto message = format_record_parse_error("Definitions.jsonc", "records", 27, "", err);
        CHECK(message.find("Definitions.jsonc: $.records[27]: ") == 0);
        CHECK(message.find(describe_parse_error(err)) != std::string::npos);
        CHECK(message.ends_with(std::to_string(code) + ")"));
    }

    CHECK(format_record_parse_error("Definitions.jsonc", "records", 0, "", ParseErrorType::GENERIC).find("$.records[0]") != std::string::npos);
}

TEST_CASE("format_json_syntax_error keeps ASCII details and masks undecodable bytes")
{
    const auto ascii = format_json_syntax_error("Definitions.jsonc", "parse error at line 3");
    CHECK(ascii.find("Definitions.jsonc: ") == 0);
    CHECK(ascii.ends_with(": parse error at line 3"));

    // 例外の説明の抜粋で、UTF-8 の文字が途中で切れた場合
    // 置き換えた '?' の後ろに記号を続けるとトライグラフになるので、文字列の末尾に置く
    const auto truncated = format_json_syntax_error("Definitions.jsonc", "last read: \xe3\x81");
    CHECK(truncated.ends_with(": last read: ??"));
}

TEST_CASE("format_record_parse_error adds the record identifier after the position")
{
    const auto with_id = format_record_parse_error("Definitions.jsonc", "records", 3, "id: \"12a\"", ParseErrorType::INVALID_VALUE);
    CHECK(with_id.find("Definitions.jsonc: $.records[3] (id: \"12a\"): ") == 0);
    CHECK(with_id.find(describe_parse_error(ParseErrorType::INVALID_VALUE)) != std::string::npos);
    CHECK(with_id.ends_with(std::to_string(enum2i(ParseErrorType::INVALID_VALUE)) + ")"));
}

TEST_CASE("describe_record_identifier prints the id as written")
{
    CHECK(describe(nlohmann::json{ { "id", 12 }, { "name", "Twelve" } }) == "id: 12");
    CHECK(describe(nlohmann::json{ { "id", -1 } }) == "id: -1");

    // 整数でない・範囲外の値も、型を問わず JSON の表記のまま出す
    CHECK(describe(nlohmann::json{ { "id", "12a" } }) == "id: \"12a\"");
    CHECK(describe(nlohmann::json{ { "id", 70000 } }) == "id: 70000");
    CHECK(describe(nlohmann::json{ { "id", 1.5 } }) == "id: 1.5");
    CHECK(describe(nlohmann::json{ { "id", nullptr } }) == "id: null");
    CHECK(describe(nlohmann::json::parse(R"({"id": [1, 2]})")) == "id: [1,2]");
}

TEST_CASE("describe_record_identifier falls back to a string name")
{
    CHECK(describe(nlohmann::json{ { "name", "MAGE" } }) == "name: \"MAGE\"");

    // 文字列でない name (日英の名前のオブジェクトなど) は識別子にしない
    CHECK(describe(nlohmann::json::parse(R"({"name": {"ja": "a", "en": "b"}})")).empty());
    CHECK(describe(nlohmann::json{ { "name", 3 } }).empty());
}

TEST_CASE("describe_record_identifier is empty without an id or a name")
{
    CHECK(describe(nlohmann::json::object()).empty());
    CHECK(describe(nlohmann::json{ { "id_list", { 8 } } }).empty());
    CHECK(describe(nlohmann::json::array({ 1, 2 })).empty());
    CHECK(describe(nlohmann::json(12)).empty());
}

TEST_CASE("describe_record_identifier truncates long values")
{
    // 表記 (引用符を含む) が40文字までなら、そのまま出す
    const std::string fits(38, 'a');
    CHECK(describe(nlohmann::json{ { "id", fits } }) == "id: \"" + fits + "\"");

    const std::string longer(60, 'a');
    CHECK(describe(nlohmann::json{ { "id", longer } }) == "id: \"" + std::string(39, 'a') + "...");
}

TEST_CASE("describe_record_identifier converts non-ASCII names to the system encoding")
{
    // ソースの文字コードに依らないよう、UTF-8 のバイト列で書く (magic は「魔法」、hiragana_a は「あ」)
    const std::string magic = "\xe9\xad\x94\xe6\xb3\x95";
    const std::string hiragana_a = "\xe3\x81\x82";
    CHECK(describe(nlohmann::json{ { "name", magic } }) == "name: " + utf8_to_sys("\"" + magic + "\"").value());

    // 長い値は文字の途中で切らず、先頭の40文字 (引用符を含む) に "..." を付ける
    std::string long_name;
    std::string kept;
    for (auto i = 0; i < 50; ++i) {
        long_name += hiragana_a;
        if (i < 39) {
            kept += hiragana_a;
        }
    }
    CHECK(describe(nlohmann::json{ { "name", long_name } }) == "name: " + utf8_to_sys("\"" + kept).value() + "...");
}

TEST_CASE("find_record_identifier is not affected by fields the parser adds later")
{
    // 解析器が operator[] で "id" を null として足しても、先に探した "name" を出す
    auto record = nlohmann::json{ { "name", "MAGE" } };
    const auto identifier = find_record_identifier(record);
    record["id"];
    record["spell_type"];
    CHECK(describe_record_identifier(identifier) == "name: \"MAGE\"");
    CHECK(describe_record_identifier(RecordIdentifier{}).empty());
}
