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
    // UTF-8 としても正しい 2 バイト (U+0122) だが、内部文字コードでは 2 桁の文字列
    // (EUC-JP では 1 文字の漢字、Shift_JIS では 2 文字の半角カナ、英語版では 2 文字の 1 バイトの文字)
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

TEST_CASE("mb_char_byte_length returns 2 for multibyte characters")
{
    using test::KANJI_KAN;
    CHECK(mb_char_byte_length(KANJI_KAN[0]) == 2);
    CHECK(is_mb_char_lead(KANJI_KAN[0]));
}

TEST_CASE("mb_char_length_at returns the length of a complete multibyte character")
{
    const auto str = test::cat("a", test::KANJI_KAN, "b");
    CHECK(mb_char_length_at(str, 0) == 1);
    CHECK(mb_char_length_at(str, 1) == 2);
    CHECK(mb_char_length_at(str, 3) == 1);
}

TEST_CASE("mb_char_length_at treats a truncated character as one byte")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(mb_char_length_at(str, 1) == 1);
}

TEST_CASE("mb_valid_char_length_at returns the length of a valid multibyte character")
{
    const auto str = test::cat("a", test::KANJI_KAN, "b");
    CHECK(mb_valid_char_length_at(str, 1) == 2);
    CHECK(mb_valid_char_length_at(str, 3) == 1);
}

TEST_CASE("mb_valid_char_length_at treats a truncated or invalid sequence as one byte")
{
    const auto lead = test::KANJI_KAN.substr(0, 1);
    CHECK(mb_valid_char_length_at(lead, 0) == 1);
    CHECK(mb_valid_char_length_at(test::cat(lead, ":"), 0) == 1);
}

TEST_CASE("is_multibyte_char_at finds a two byte character with its trailing byte")
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
    const auto str = test::cat("a", test::KANJI_KAN, test::KANJI_JI, "b");
    CHECK(mb_char_start_pos(str, 1) == 1);
    CHECK(mb_char_start_pos(str, 2) == 1);
    CHECK(mb_char_start_pos(str, 3) == 3);
    CHECK(mb_char_start_pos(str, 4) == 3);
    CHECK(mb_char_start_pos(str, 5) == 5);
    CHECK(mb_char_start_pos(str, 6) == 6);
}

TEST_CASE("mb_char_start_pos returns the length for the end of a truncated character")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(mb_char_start_pos(str, 1) == 1);
    CHECK(mb_char_start_pos(str, 2) == 2);
}

TEST_CASE("mb_next_char_boundary moves forward to the end of a multibyte character")
{
    const auto str = test::cat("a", test::KANJI_KAN, test::KANJI_JI, "b");
    CHECK(mb_next_char_boundary(str, 1) == 1);
    CHECK(mb_next_char_boundary(str, 2) == 3);
    CHECK(mb_next_char_boundary(str, 3) == 3);
    CHECK(mb_next_char_boundary(str, 4) == 5);
    CHECK(mb_next_char_boundary(str, 6) == 6);
}

TEST_CASE("mb_next_char_boundary does not go beyond the end of a truncated character")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(mb_next_char_boundary(str, 2) == 2);
}

TEST_CASE("is_mb_char_boundary is false inside a multibyte character")
{
    const auto str = test::cat("a", test::KANJI_KAN, "b");
    CHECK(is_mb_char_boundary(str, 1));
    CHECK_FALSE(is_mb_char_boundary(str, 2));
    CHECK(is_mb_char_boundary(str, 3));
    CHECK(is_mb_char_boundary(str, 4));
}

TEST_CASE("mb_prev_char_pos skips back over a multibyte character")
{
    const auto str = test::cat("a", test::KANJI_KAN, test::KANJI_JI, "b");
    CHECK(mb_prev_char_pos(str, 1) == 0);
    CHECK(mb_prev_char_pos(str, 3) == 1);
    CHECK(mb_prev_char_pos(str, 5) == 3);
    CHECK(mb_prev_char_pos(str, 6) == 5);
}

TEST_CASE("mb_prev_char_pos treats a truncated character at the end as one character")
{
    const auto str = test::cat("a", test::KANJI_KAN.substr(0, 1));
    CHECK(mb_prev_char_pos(str, 2) == 1);
}
#endif

#if defined(JP) && defined(SJIS)
TEST_CASE("is_printable_single_byte_char accepts half-width katakana")
{
    CHECK(is_printable_single_byte_char('\xb1')); // 半角の「ア」
}

TEST_CASE("is_multibyte_char_at finds a character whose trailing byte is ASCII")
{
    CHECK(is_multibyte_char_at(test::DAME_SO));
    CHECK(is_multibyte_char_at(test::cat(test::DAME_SO, "a").data()));
}

TEST_CASE("mb_char_start_pos does not treat an ASCII-valued trailing byte as a character start")
{
    // ダメ文字の後半バイトは ASCII の文字と同じ値なので、文字の境界は先頭から数えて求める
    const auto str = test::cat(test::DAME_SO, test::DAME_CHOON, test::DAME_KANA_A);
    CHECK(mb_char_start_pos(str, 1) == 0);
    CHECK(mb_char_start_pos(str, 3) == 2);
    CHECK(mb_char_start_pos(str, 5) == 4);
    CHECK(mb_prev_char_pos(str, 4) == 2);
}

TEST_CASE("mb_valid_char_length_at accepts an ASCII-valued trailing byte in Shift_JIS")
{
    CHECK(mb_valid_char_length_at(test::DAME_SO, 0) == 2);
    CHECK(mb_valid_char_length_at("\xb1:", 0) == 1); // 半角の「ア」は 1 バイトの文字
}

TEST_CASE("mb_valid_char_length_at checks the range of a Shift_JIS trailing byte")
{
    CHECK(mb_valid_char_length_at("\x83\x40", 0) == 2);
    CHECK(mb_valid_char_length_at("\x83\x7e", 0) == 2);
    CHECK(mb_valid_char_length_at("\x83\x80", 0) == 2);
    CHECK(mb_valid_char_length_at("\x83\xfc", 0) == 2);
    CHECK(mb_valid_char_length_at("\x83\x3f", 0) == 1);
    CHECK(mb_valid_char_length_at("\x83\x7f", 0) == 1);
    CHECK(mb_valid_char_length_at("\x83\xfd", 0) == 1);
    CHECK(mb_valid_char_length_at("\x83\x1f", 0) == 1);
}
#elif defined(JP)
TEST_CASE("mb_valid_char_length_at handles EUC-JP three byte and half-width katakana sequences")
{
    CHECK(mb_valid_char_length_at("\x8f\xa1\xa1", 0) == 3); // JIS X 0212 の文字
    CHECK(mb_valid_char_length_at("\x8f\xa1:", 0) == 1);
    CHECK(mb_valid_char_length_at("\x8f\xa1", 0) == 1);
    CHECK(mb_valid_char_length_at("\x8f\xa1\xa0", 0) == 1);
    CHECK(mb_valid_char_length_at("\x8e\xb1", 0) == 2); // 半角の「ア」
    CHECK(mb_valid_char_length_at("\x8e\xa1\xa1", 0) == 2);
    CHECK(mb_valid_char_length_at("\x8e\xe0", 0) == 1);
}

TEST_CASE("mb_valid_char_length_at checks the range of an EUC-JP trailing byte")
{
    CHECK(mb_valid_char_length_at("\xa1\xa1", 0) == 2);
    CHECK(mb_valid_char_length_at("\xa1\xfe", 0) == 2);
    CHECK(mb_valid_char_length_at("\xa1\xa0", 0) == 1);
    CHECK(mb_valid_char_length_at("\xa1\xff", 0) == 1);
}
#endif
