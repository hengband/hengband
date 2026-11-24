/*!
 * @brief 内部文字コードの文字単位の処理のテスト
 */

#include "locale/multibyte-char.h"
#include "test/string-helpers.h"
#include <doctest/doctest.h>
#include <fmt/format.h>
#include <string>
#include <string_view>

using namespace std::string_view_literals;

TEST_CASE("mb_char_byte_length returns 1 for ASCII")
{
    CHECK(mb_char_byte_length('a') == 1);
    CHECK(mb_char_byte_length(' ') == 1);
    CHECK(mb_char_byte_length('\0') == 1);
    CHECK_FALSE(is_mb_char_lead('a'));
}

TEST_CASE("is_printable_single_byte_char accepts printable ASCII characters only")
{
    CHECK(is_printable_single_byte_char('a'));
    CHECK(is_printable_single_byte_char(' '));
    CHECK_FALSE(is_printable_single_byte_char('\n'));
    CHECK_FALSE(is_printable_single_byte_char('\x7f'));
}

TEST_CASE("is_multibyte_char_at returns false for ASCII strings")
{
    CHECK_FALSE(is_multibyte_char_at("ab"));
    CHECK_FALSE(is_multibyte_char_at(std::string_view("ab"), 0));
    CHECK_FALSE(is_multibyte_char_at(""));
    CHECK_FALSE(is_multibyte_char_at(std::string_view(), 0));
}

TEST_CASE("mb_char_length_at returns 1 for ASCII and out-of-range positions")
{
    CHECK(mb_char_length_at("abc"sv, 0) == 1);
    CHECK(mb_char_length_at("abc"sv, 2) == 1);
    CHECK(mb_char_length_at("abc"sv, 3) == 1);
}

TEST_CASE("mb_valid_char_length_at returns 1 for ASCII and out-of-range positions")
{
    CHECK(mb_valid_char_length_at("ab"sv, 0) == 1);
    CHECK(mb_valid_char_length_at("ab"sv, 2) == 1);
    CHECK(mb_valid_char_length_at("ab"sv, 5) == 1);
}

TEST_CASE("mb_first_char returns empty for an empty string")
{
    CHECK(mb_first_char(""sv).empty());
}

TEST_CASE("mb_first_char returns the first ASCII character")
{
    CHECK(mb_first_char("abc"sv) == "a"sv);
}

TEST_CASE("mb_char_start_pos returns the position itself for ASCII")
{
    CHECK(mb_char_start_pos("abc"sv, 0) == 0);
    CHECK(mb_char_start_pos("abc"sv, 2) == 2);
    CHECK(mb_char_start_pos("abc"sv, 3) == 3);
    CHECK(mb_char_start_pos("abc"sv, 10) == 3);
}

TEST_CASE("mb_next_char_boundary returns the position itself for ASCII")
{
    CHECK(mb_next_char_boundary("abc"sv, 0) == 0);
    CHECK(mb_next_char_boundary("abc"sv, 2) == 2);
    CHECK(mb_next_char_boundary("abc"sv, 3) == 3);
    CHECK(mb_next_char_boundary("abc"sv, 10) == 3);
}

TEST_CASE("is_mb_char_boundary is true for every position of an ASCII string up to its length")
{
    CHECK(is_mb_char_boundary("abc"sv, 0));
    CHECK(is_mb_char_boundary("abc"sv, 2));
    CHECK(is_mb_char_boundary("abc"sv, 3));
    CHECK_FALSE(is_mb_char_boundary("abc"sv, 4));
}

TEST_CASE("mb_prev_char_pos returns the start of the previous ASCII character")
{
    CHECK(mb_prev_char_pos("abc"sv, 0) == 0);
    CHECK(mb_prev_char_pos("abc"sv, 1) == 0);
    CHECK(mb_prev_char_pos("abc"sv, 3) == 2);
    CHECK(mb_prev_char_pos("abc"sv, 10) == 2);
}

TEST_CASE("str_display_width counts an ASCII character as one column")
{
    CHECK(str_display_width(""sv) == 0);
    CHECK(str_display_width("abc"sv) == 3);
}

TEST_CASE("str_prefix_by_width returns the prefix that fits in the width")
{
    CHECK(str_prefix_by_width("abcdef"sv, 3) == "abc"sv);
    CHECK(str_prefix_by_width("ab"sv, 3) == "ab"sv);
    CHECK(str_prefix_by_width("abc"sv, 0).empty());
}

TEST_CASE("str_prefix_by_width shrinks a temporary string and returns it")
{
    const std::string result = str_prefix_by_width(std::string("abcdef"), 3);
    CHECK(result == "abc");
}

TEST_CASE("fmt pads strings by str_display_width")
{
    // UTF-8 として正しい 2 バイト (U+0122) で、内部文字コードでは 2 桁の文字列
    // (日本語版では 2 桁の 1 文字、英語版では 2 文字の 1 バイトの文字)
    const auto str = "\xc4\xa2"sv;
    CHECK(str_display_width(str) == 2);
    CHECK(fmt::format("{:<4}|", str) == test::cat(str, "  |"));
    CHECK(fmt::format("{:>4}|", str) == test::cat("  ", str, "|"));
    CHECK(fmt::format("{:<{}}|", str, 3) == test::cat(str, " |"));
}

#ifdef JP
TEST_CASE("str_display_width counts a multibyte character as two columns")
{
    using test::KANJI_KAN;
    CHECK(str_display_width(test::cat("a", KANJI_KAN, test::KANJI_JI)) == 5);
    CHECK(fmt::format("{:<6}|", KANJI_KAN) == test::cat(KANJI_KAN, "    |"));
}

TEST_CASE("str_prefix_by_width does not split a multibyte character")
{
    using test::KANJI_KAN;
    const auto str = test::cat("a", KANJI_KAN, "b");
    CHECK(str_prefix_by_width(str, 1) == "a"sv);
    CHECK(str_prefix_by_width(str, 2) == "a"sv);
    CHECK(str_prefix_by_width(str, 3) == test::cat("a", KANJI_KAN));
    CHECK(str_prefix_by_width(str, 4) == str);
}

TEST_CASE("str_prefix_by_width drops a truncated character at the end")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(str_prefix_by_width(str, 10) == "a"sv);
}

TEST_CASE("str_display_width counts a truncated character as one column")
{
    CHECK(str_display_width(test::cat("a", test::KANJI_KAN.substr(0, 1))) == 2);
}

TEST_CASE("mb_char_byte_length returns the byte length of a multibyte character")
{
    using test::KANJI_KAN;
    CHECK(mb_char_byte_length(KANJI_KAN[0]) == KANJI_KAN.length());
    CHECK(is_mb_char_lead(KANJI_KAN[0]));
}

TEST_CASE("mb_char_length_at returns the length of a complete multibyte character")
{
    const auto kan_length = test::KANJI_KAN.length();
    const auto str = test::cat("a", test::KANJI_KAN, "b");
    CHECK(mb_char_length_at(str, 0) == 1);
    CHECK(mb_char_length_at(str, 1) == kan_length);
    CHECK(mb_char_length_at(str, 1 + kan_length) == 1);
}

TEST_CASE("mb_char_length_at treats a truncated character as one byte")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(mb_char_length_at(str, 1) == 1);
}

TEST_CASE("mb_valid_char_length_at returns the length of a valid multibyte character")
{
    const auto kan_length = test::KANJI_KAN.length();
    const auto str = test::cat("a", test::KANJI_KAN, "b");
    CHECK(mb_valid_char_length_at(str, 1) == kan_length);
    CHECK(mb_valid_char_length_at(str, 1 + kan_length) == 1);
}

TEST_CASE("mb_valid_char_length_at treats a truncated or invalid sequence as one byte")
{
    const auto lead = test::KANJI_KAN.substr(0, 1);
    CHECK(mb_valid_char_length_at(lead, 0) == 1);
    CHECK(mb_valid_char_length_at(test::cat(lead, ":"), 0) == 1);
}

TEST_CASE("is_multibyte_char_at finds a multibyte character with its trailing bytes")
{
    const auto str = test::cat("a", test::KANJI_KAN);
    CHECK_FALSE(is_multibyte_char_at(str));
    CHECK(is_multibyte_char_at(str, 1));
    CHECK(is_multibyte_char_at(test::KANJI_KAN));
    CHECK(is_multibyte_char_at(str.data() + 1));
}

TEST_CASE("is_multibyte_char_at rejects a leading byte without its trailing byte")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK_FALSE(is_multibyte_char_at(str, 1));
    CHECK_FALSE(is_multibyte_char_at(str.data() + 1));

    // 範囲外の位置は多バイト文字の先頭ではない
    CHECK_FALSE(is_multibyte_char_at(str, 2));
    CHECK_FALSE(is_multibyte_char_at(str, std::string_view::npos));
}

TEST_CASE("mb_first_char returns the first multibyte character")
{
    using test::KANJI_KAN;
    CHECK(mb_first_char(test::cat(KANJI_KAN, test::KANJI_JI)) == KANJI_KAN);
}

TEST_CASE("mb_first_char returns only the lead byte of a truncated character")
{
    const auto lead = test::KANJI_KAN.substr(0, 1);
    CHECK(mb_first_char(lead) == lead);
}

TEST_CASE("mb_char_start_pos moves back to the start of a multibyte character")
{
    const auto ji_pos = 1 + test::KANJI_KAN.length();
    const auto b_pos = ji_pos + test::KANJI_JI.length();
    const auto str = test::cat("a", test::KANJI_KAN, test::KANJI_JI, "b");
    for (size_t pos = 1; pos < ji_pos; pos++) {
        CAPTURE(pos);
        CHECK(mb_char_start_pos(str, pos) == 1);
    }

    for (auto pos = ji_pos; pos < b_pos; pos++) {
        CAPTURE(pos);
        CHECK(mb_char_start_pos(str, pos) == ji_pos);
    }

    CHECK(mb_char_start_pos(str, b_pos) == b_pos);
    CHECK(mb_char_start_pos(str, str.length()) == str.length());
}

TEST_CASE("mb_char_start_pos returns the length for the end of a truncated character")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(mb_char_start_pos(str, 1) == 1);
    CHECK(mb_char_start_pos(str, 2) == 2);
}

TEST_CASE("mb_next_char_boundary moves forward to the end of a multibyte character")
{
    const auto ji_pos = 1 + test::KANJI_KAN.length();
    const auto b_pos = ji_pos + test::KANJI_JI.length();
    const auto str = test::cat("a", test::KANJI_KAN, test::KANJI_JI, "b");
    CHECK(mb_next_char_boundary(str, 1) == 1);
    CHECK(mb_next_char_boundary(str, 2) == ji_pos);
    CHECK(mb_next_char_boundary(str, ji_pos) == ji_pos);
    CHECK(mb_next_char_boundary(str, ji_pos + 1) == b_pos);
    CHECK(mb_next_char_boundary(str, str.length()) == str.length());
}

TEST_CASE("mb_next_char_boundary does not go beyond the end of a truncated character")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(mb_next_char_boundary(str, 2) == 2);
}

TEST_CASE("is_mb_char_boundary is false inside a multibyte character")
{
    const auto b_pos = 1 + test::KANJI_KAN.length();
    const auto str = test::cat("a", test::KANJI_KAN, "b");
    CHECK(is_mb_char_boundary(str, 1));
    CHECK_FALSE(is_mb_char_boundary(str, 2));
    CHECK(is_mb_char_boundary(str, b_pos));
    CHECK(is_mb_char_boundary(str, b_pos + 1));
}

TEST_CASE("mb_prev_char_pos skips back over a multibyte character")
{
    const auto ji_pos = 1 + test::KANJI_KAN.length();
    const auto b_pos = ji_pos + test::KANJI_JI.length();
    const auto str = test::cat("a", test::KANJI_KAN, test::KANJI_JI, "b");
    CHECK(mb_prev_char_pos(str, 1) == 0);
    CHECK(mb_prev_char_pos(str, ji_pos) == 1);
    CHECK(mb_prev_char_pos(str, b_pos) == ji_pos);
    CHECK(mb_prev_char_pos(str, str.length()) == b_pos);
}

TEST_CASE("mb_prev_char_pos treats a truncated character at the end as one character")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(mb_prev_char_pos(str, 2) == 1);
}
#endif

#if defined(JP) && defined(UTF8)
TEST_CASE("mb_char_byte_length reads the length from a UTF-8 lead byte")
{
    CHECK(mb_char_byte_length('\xc3') == 2);
    CHECK(mb_char_byte_length('\xe3') == 3);
    CHECK(mb_char_byte_length('\xf0') == 4);
    CHECK(mb_char_byte_length('\xc2') == 2);
    CHECK(mb_char_byte_length('\xef') == 3);
    CHECK(mb_char_byte_length('\xf4') == 4);

    // 後続のバイトと、先頭に現れない不正なバイトは 1 バイトの文字とみなす
    CHECK(mb_char_byte_length('\x81') == 1);
    CHECK(mb_char_byte_length('\xc0') == 1);
    CHECK(mb_char_byte_length('\xc1') == 1);
    CHECK(mb_char_byte_length('\xf5') == 1);
    CHECK(mb_char_byte_length('\xff') == 1);
}

TEST_CASE("mb_valid_char_length_at checks the trailing bytes of a UTF-8 character")
{
    CHECK(mb_valid_char_length_at("\xc3\xa9", 0) == 2); // é
    CHECK(mb_valid_char_length_at("\xe3\x81\x82", 0) == 3); // あ
    CHECK(mb_valid_char_length_at("\xf0\x9f\x98\x80", 0) == 4); // U+1F600
    CHECK(mb_valid_char_length_at("\xe3\x81:", 0) == 1);
    CHECK(mb_valid_char_length_at("\xe3\x81", 0) == 1);
}

TEST_CASE("str_display_width counts every non-ASCII UTF-8 character as two columns")
{
    CHECK(str_display_width("a\xc3\xa9"sv) == 3); // é
    CHECK(str_display_width("\xef\xbd\xb1"sv) == 2); // 半角の「ア」も 2 桁
    CHECK(str_display_width("\xf0\x9f\x98\x80"sv) == 2); // U+1F600
}
#endif
