/*!
 * @brief ゲームデータ解析エラー名の表のテスト
 */

#include "info-reader/parse-error-types.h"
#include "main/init-error-messages-table.h"
#include "util/enum-converter.h"
#include <doctest/doctest.h>
#include <string>

TEST_CASE("err_str has a message for every parse error")
{
    // ParseErrorType::NONE (0) はエラーではないので名称を持たない
    for (auto err = 1; err < enum2i(ParseErrorType::MAX); err++) {
        CAPTURE(err);
        CHECK(err_str[err] != nullptr);
    }
}

TEST_CASE("describe_parse_error names every parse error")
{
    for (auto code = enum2i(ParseErrorType::GENERIC); code < enum2i(ParseErrorType::MAX); ++code) {
        CAPTURE(code);
        const auto err = static_cast<ParseErrorType>(code);
        CHECK_FALSE(describe_parse_error(err).empty());
        CHECK(describe_parse_error(err) == err_str[code]);
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
