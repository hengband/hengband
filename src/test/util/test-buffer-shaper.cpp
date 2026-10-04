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
#elif defined(JP)
constexpr std::string_view KUTEN = "\xa1\xa3"; //!< 。
#endif

}

TEST_CASE("shape_buffer returns a short string as is")
{
    CHECK(shape_buffer("abc", 10) == Lines{ "abc" });
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

#ifdef JP
TEST_CASE("shape_buffer splits multibyte characters so that each line fits in the width")
{
    CHECK(shape_buffer(repeat(KANJI_KAN, 6), 10) == Lines{ repeat(KANJI_KAN, 4), repeat(KANJI_KAN, 2) });
}

TEST_CASE("shape_buffer does not put a kinsoku character at the head of a line")
{
    const auto str = cat(repeat(KANJI_KAN, 4), KUTEN, KANJI_KAN);
    CHECK(shape_buffer(str, 10) == Lines{ repeat(KANJI_KAN, 3), cat(KANJI_KAN, KUTEN, KANJI_KAN) });
}
#endif
