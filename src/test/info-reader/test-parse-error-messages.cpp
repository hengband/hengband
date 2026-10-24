/*!
 * @brief ゲームデータ解析エラー名の表のテスト
 */

#include "info-reader/parse-error-messages.h"
#include "info-reader/parse-error-types.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <string>
#include <vector>

TEST_CASE("format_definition_load_error preserves diagnostics before the record error")
{
    const auto record = format_record_parse_error("Definitions.jsonc", "records", 3, ParseErrorType::INVALID_FLAG);
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
        const auto message = format_record_parse_error("Definitions.jsonc", "records", 27, err);
        CHECK(message.find("Definitions.jsonc: $.records[27]: ") == 0);
        CHECK(message.find(describe_parse_error(err)) != std::string::npos);
        CHECK(message.ends_with(std::to_string(code) + ")"));
    }

    CHECK(format_record_parse_error("Definitions.jsonc", "records", 0, ParseErrorType::GENERIC).find("$.records[0]") != std::string::npos);
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
