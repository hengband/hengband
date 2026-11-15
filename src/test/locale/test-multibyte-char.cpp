/*!
 * @brief 内部文字コードの文字単位の処理のテスト
 */

#include "locale/multibyte-char.h"
#include "test/string-helpers.h"
#include <doctest/doctest.h>
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

#ifdef JP
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
#endif
