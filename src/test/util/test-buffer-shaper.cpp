/*!
 * @brief 文字列の分割のテスト
 */

#include "util/buffer-shaper.h"

#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

using namespace test;

namespace {

using Lines = std::vector<std::string>;

#if defined(JP) && defined(SJIS)
constexpr std::string_view KUTEN = "\x81\x42"; //!< 。
constexpr std::string_view CHOON = "\x81\x5b"; //!< ー
#elif defined(JP)
constexpr std::string_view KUTEN = "\xa1\xa3"; //!< 。
constexpr std::string_view CHOON = "\xa1\xbc"; //!< ー
#endif

}

TEST_CASE("classify_line_break allows a break after a space and in the middle of a half-width word")
{
    CHECK(classify_line_break(" ", "a") == LineBreakKind::WORD_BREAK);
    CHECK(classify_line_break("a", "b") == LineBreakKind::MID_WORD);
}

TEST_CASE("classify_line_break does not allow a break at the head, before a space or before a closing punctuation in a word")
{
    CHECK(classify_line_break("", "a") == LineBreakKind::NONE);
    CHECK(classify_line_break("a", "") == LineBreakKind::NONE);
    CHECK(classify_line_break("a", " ") == LineBreakKind::NONE);
    CHECK(classify_line_break("a", ")") == LineBreakKind::NONE);
    CHECK(classify_line_break("a", ",") == LineBreakKind::NONE);
}

TEST_CASE("shape_buffer returns a short string as is")
{
    CHECK(shape_buffer("abc", 10) == Lines{ "abc" });
}

TEST_CASE("shape_buffer keeps a string shorter than the width by one in one line")
{
    // 終端文字の分を残すので、1 行は最大 maxlen - 1 バイト
    CHECK(shape_buffer("abcd", 5) == Lines{ "abcd" });
    CHECK(shape_buffer("abcde", 5) == Lines{ "abcd", "e" });
}

TEST_CASE("shape_buffer replaces an unprintable character with a space")
{
    CHECK(shape_buffer("a\tb", 10) == Lines{ "a b" });
}

TEST_CASE("shape_buffer returns no lines for an empty string")
{
    CHECK(shape_buffer("", 10).empty());
}

TEST_CASE("shape_buffer splits a string at newlines")
{
    CHECK(shape_buffer("ab\ncd", 10) == Lines{ "ab", "cd" });
}

TEST_CASE("shape_buffer keeps an empty line between newlines but adds none after a trailing newline")
{
    CHECK(shape_buffer("a\n\nb", 10) == Lines{ "a", "", "b" });
    CHECK(shape_buffer("ab\n", 10) == Lines{ "ab" });
}

TEST_CASE("shape_buffer splits a string at a space and removes the space at the head of the next line")
{
    CHECK(shape_buffer("aaaa bbbb cccc", 10) == Lines{ "aaaa bbbb", "cccc" });
}

TEST_CASE("shape_buffer splits a string at the last space if the space is in the second half")
{
    // 長い単語は次の行に送り、その行にも収まらない部分は単語の途中で分ける
    CHECK(shape_buffer("aaaaa bbbbbbbbbb", 10) == Lines{ "aaaaa", "bbbbbbbbb", "b" });
}

TEST_CASE("shape_buffer splits a long word in the middle if the last space is in the first half")
{
    CHECK(shape_buffer("aaaa bbbbbbbbbb", 10) == Lines{ "aaaa bbbb", "bbbbbb" });
}

TEST_CASE("shape_buffer puts at least one character in each line even if no character fits in the width")
{
    CHECK(shape_buffer("abc", 1) == Lines{ "a", "b", "c" });
    CHECK(shape_buffer("abc", 0) == Lines{ "a", "b", "c" });
    CHECK(shape_buffer("a\nb", 1) == Lines{ "a", "b" });
}

#ifdef JP
TEST_CASE("classify_line_break allows a break before and after a multibyte character")
{
    CHECK(classify_line_break("a", KANJI_KAN) == LineBreakKind::WORD_BREAK);
    CHECK(classify_line_break(KANJI_KAN, "a") == LineBreakKind::WORD_BREAK);
    CHECK(classify_line_break(KANJI_KAN, ")") == LineBreakKind::WORD_BREAK);
    CHECK(classify_line_break(KANJI_KAN, KANJI_JI) == LineBreakKind::WORD_BREAK);
}

TEST_CASE("classify_line_break does not allow a break before a kinsoku character")
{
    CHECK(classify_line_break(KANJI_KAN, KUTEN) == LineBreakKind::NONE);
    CHECK(classify_line_break("a", KUTEN) == LineBreakKind::NONE);
    CHECK(classify_line_break(" ", KUTEN) == LineBreakKind::NONE);
}

TEST_CASE("classify_line_break does not allow a break after an opening bracket")
{
    CHECK(classify_line_break(KAKKO_OPEN, KANJI_KAN) == LineBreakKind::NONE);
    CHECK(classify_line_break("(", KANJI_KAN) == LineBreakKind::NONE);
    CHECK(classify_line_break(KANJI_KAN, KAKKO_OPEN) == LineBreakKind::WORD_BREAK);
}

TEST_CASE("needs_line_break_backtrack is true before a kinsoku character and after an opening bracket")
{
    CHECK(needs_line_break_backtrack(KANJI_KAN, KUTEN));
    CHECK(needs_line_break_backtrack(KAKKO_OPEN, KANJI_KAN));
    CHECK(needs_line_break_backtrack("(", "a"));
}

TEST_CASE("needs_line_break_backtrack is false where a break is allowed, at the head or before a half-width closing punctuation")
{
    CHECK_FALSE(needs_line_break_backtrack(KANJI_KAN, KANJI_JI));
    CHECK_FALSE(needs_line_break_backtrack("", KANJI_KAN));
    CHECK_FALSE(needs_line_break_backtrack(KAKKO_OPEN, ""));
    CHECK_FALSE(needs_line_break_backtrack("a", "b"));
    CHECK_FALSE(needs_line_break_backtrack("a", ")"));
}

TEST_CASE("shape_buffer splits before a half-width opening bracket at the end of a line in a word")
{
    // 半角の始め括弧で行が終わるときは、語の途中でも括弧の前で分ける
    CHECK(shape_buffer("abcdefgh(ij", 10) == Lines{ "abcdefgh", "(ij" });
}

TEST_CASE("shape_buffer splits in a word after a half-width opening bracket if the word break is in the first half")
{
    // 行末禁則文字でさかのぼるときも、語の切れ目が行の前半にしか無ければ、それより後ろの語の途中 (始め括弧の前) で分ける
    CHECK(shape_buffer(cat(KANJI_KAN, " aaaaaa(b"), 11) == Lines{ cat(KANJI_KAN, " aaaaaa"), "(b" });
}

TEST_CASE("shape_buffer does not leave an opening bracket at the end of a line")
{
    // 始め括弧の直後では分けず、始め括弧の前の語の切れ目で分ける
    CHECK(shape_buffer(cat(repeat(KANJI_KAN, 3), KAKKO_OPEN, repeat(KANJI_KAN, 2)), 10) == Lines{ repeat(KANJI_KAN, 3), cat(KAKKO_OPEN, repeat(KANJI_KAN, 2)) });
    CHECK(shape_buffer(cat(repeat(KANJI_KAN, 3), "a(", repeat(KANJI_KAN, 2)), 10) == Lines{ repeat(KANJI_KAN, 3), cat("a(", repeat(KANJI_KAN, 2)) });
}

TEST_CASE("shape_buffer puts at least one multibyte character in each line even if it does not fit in the width")
{
    CHECK(shape_buffer(repeat(KANJI_KAN, 2), 2) == Lines{ std::string(KANJI_KAN), std::string(KANJI_KAN) });
    CHECK(shape_buffer(repeat(KANJI_KAN, 2), 1) == Lines{ std::string(KANJI_KAN), std::string(KANJI_KAN) });
    CHECK(shape_buffer(cat("a", KANJI_KAN), 2) == Lines{ "a", std::string(KANJI_KAN) });
}

TEST_CASE("shape_buffer returns a short multibyte string as is")
{
    CHECK(shape_buffer(repeat(KANJI_KAN, 2), 10) == Lines{ repeat(KANJI_KAN, 2) });
}

TEST_CASE("shape_buffer replaces a lead byte at the end with a space")
{
    // 全角文字の前半バイトだけで終わっていたら、表示できない文字として空白にする
    CHECK(shape_buffer(cat("ab", KANJI_KAN.substr(0, 1)), 10) == Lines{ "ab " });
}

TEST_CASE("shape_buffer splits multibyte characters so that each line fits in the width")
{
    CHECK(shape_buffer(repeat(KANJI_KAN, 6), 10) == Lines{ repeat(KANJI_KAN, 4), repeat(KANJI_KAN, 2) });
}

TEST_CASE("shape_buffer does not put a kinsoku character at the head of a line")
{
    const auto str = cat(repeat(KANJI_KAN, 4), KUTEN, KANJI_KAN);
    CHECK(shape_buffer(str, 10) == Lines{ repeat(KANJI_KAN, 3), cat(KANJI_KAN, KUTEN, KANJI_KAN) });
}

TEST_CASE("shape_buffer goes back over consecutive kinsoku characters")
{
    const auto str = cat(repeat(KANJI_KAN, 3), KUTEN, KUTEN, KANJI_KAN);
    CHECK(shape_buffer(str, 10) == Lines{ repeat(KANJI_KAN, 2), cat(KANJI_KAN, KUTEN, KUTEN, KANJI_KAN) });
}

TEST_CASE("shape_buffer does not split at a space before a kinsoku character")
{
    // 空白の後ろで分けると次の行が禁則文字で始まるので、その前の語の切れ目で分ける
    const auto str = cat(repeat(KANJI_KAN, 3), "a ", KUTEN);
    CHECK(shape_buffer(str, 10) == Lines{ repeat(KANJI_KAN, 3), cat("a ", KUTEN) });
}

TEST_CASE("shape_buffer does not split at a space that reaches the width before a kinsoku character")
{
    // 最大長に達した空白で分けると次の行が禁則文字で始まるので、その前の語の切れ目で分ける
    const auto str = cat(repeat(KANJI_KAN, 4), "! ", KUTEN);
    CHECK(shape_buffer(str, 10) == Lines{ repeat(KANJI_KAN, 4), cat("! ", KUTEN) });
}

TEST_CASE("shape_buffer splits a half-width word before a kinsoku character when there is no word break")
{
    // 語の切れ目が無ければ、半角の単語の途中で分ける
    CHECK(shape_buffer(cat("abcdefgh", KUTEN), 10) == Lines{ "abcdefg", cat("h", KUTEN) });
}

TEST_CASE("shape_buffer does not split a half-width word before a closing punctuation")
{
    // 語の途中で分けるときは、半角の閉じ括弧の前では分けない
    CHECK(shape_buffer(cat("abcdefghij(150)", KUTEN), 16) == Lines{ "abcdefghij(15", cat("0)", KUTEN) });
}

TEST_CASE("shape_buffer splits at a word break in the first half if it is after the last position in a word")
{
    // 語の切れ目が行の前半にしか無くても、それより後ろに語の途中の位置が無ければ語の切れ目で分ける
    CHECK(shape_buffer(cat("abc", KANJI_KAN, repeat(CHOON, 5)), 15) == Lines{ "abc", cat(KANJI_KAN, repeat(CHOON, 5)) });
}

TEST_CASE("shape_buffer does not add a line of only spaces when the line starts with spaces")
{
    // 先頭の空白の直後しか語の切れ目が無ければ、空白だけの行を作らずに今の位置で分ける
    CHECK(shape_buffer(cat("  ", KANJI_KAN, repeat(KUTEN, 6)), 10) == Lines{ cat("  ", KANJI_KAN, repeat(KUTEN, 2)), repeat(KUTEN, 4) });
}

TEST_CASE("shape_buffer does not add an empty line when the only word break is after a leading space")
{
    // 先頭の空白の直後しか語の切れ目が無ければ、空の行を作らずに今の位置で分ける
    CHECK(shape_buffer(cat(" ", KANJI_KAN, repeat(KUTEN, 6)), 10) == Lines{ cat(" ", KANJI_KAN, repeat(KUTEN, 3)), repeat(KUTEN, 3) });
}

TEST_CASE("shape_buffer keeps a multibyte character together with the following half-width character at the width")
{
    // 全角文字の直後の半角文字で最大長に達したときは、これまでどおり全角文字の前で分ける
    CHECK(shape_buffer(cat("abcdefg", KANJI_KAN, "h"), 10) == Lines{ "abcdefg", cat(KANJI_KAN, "h") });
}
#else
TEST_CASE("shape_buffer keeps a half-width opening bracket at the end of a line in the English version")
{
    // 英語版では半角の始め括弧を行末禁則文字として扱わず、これまでどおり今の位置で分ける
    CHECK_FALSE(needs_line_break_backtrack("(", "a"));
    CHECK(shape_buffer("abcdefgh(ij", 10) == Lines{ "abcdefgh(", "ij" });
}
#endif
