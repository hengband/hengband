/*!
 * @brief 画面への文字列の書き出しのテスト
 *
 * c_roff() が端末の右端で折り返すときの、折り返す位置と行頭禁則を検証する。
 * 2バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
 */

#include "term/screen-processor.h"

#include "term/term-color-types.h"
#include "term/z-term.h"
#include "test/scoped-input-term.h"
#include "test/string-helpers.h"
#include "util/string-processor.h"
#include "view/display-symbol.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>

using namespace test;

namespace {
#if defined(JP) && defined(SJIS)
constexpr std::string_view TOUTEN = "\x81\x41"; //!< 、
#elif defined(JP)
constexpr std::string_view TOUTEN = "\xa1\xa2"; //!< 、
#endif

/*!
 * @brief 端末の指定した行の文字を、末尾の空白を除いて返す
 * @param y 行
 * @return 行の文字
 */
std::string read_screen_line(int y)
{
    const auto &[wid, hgt] = term_get_size();
    std::string line;
    for (auto x = 0; x < wid; x++) {
        DisplaySymbol ds;
        ds = term_what(x, y, ds);
        line.push_back(ds.character);
    }

    return str_rtrim(line);
}
}

TEST_CASE("c_roff wraps a word longer than the line at the right edge without copying the line")
{
    // 空白の無い 85 文字は、右端の 1 桁を残した 79 桁で折り返し、残りの 6 文字だけを次の行に書く
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, std::string(85, 'a'));
    CHECK(read_screen_line(0) == std::string(79, 'a'));
    CHECK(read_screen_line(1) == std::string(6, 'a'));
}

#ifdef JP

TEST_CASE("c_roff moves only a half-width character before a kinsoku character to the next line")
{
    // 80桁の端末で、漢字39文字 (78桁) と半角文字の後の読点が右端に来ると、半角文字だけを読点と一緒に次の行へ送る
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(repeat(KANJI_KAN, 39), "A", TOUTEN, KANJI_KAN));
    CHECK(read_screen_line(0) == repeat(KANJI_KAN, 39));
    CHECK(read_screen_line(1) == cat("A", TOUTEN, KANJI_KAN));
}

TEST_CASE("c_roff moves a full-width character before a kinsoku character to the next line")
{
    // 漢字38文字 (76桁) と別の漢字の後の読点が右端に来ると、その漢字を読点と一緒に次の行へ送る
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(repeat(KANJI_KAN, 38), KANJI_JI, TOUTEN, KANJI_KAN));
    CHECK(read_screen_line(0) == repeat(KANJI_KAN, 38));
    CHECK(read_screen_line(1) == cat(KANJI_JI, TOUTEN, KANJI_KAN));
}

TEST_CASE("c_roff moves the characters before consecutive kinsoku characters to the next line")
{
    // 行頭禁則文字が続くときは、続く禁則文字の前の文字までさかのぼって送る
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(repeat(KANJI_KAN, 37), KANJI_JI, TOUTEN, TOUTEN, KANJI_KAN));
    CHECK(read_screen_line(0) == repeat(KANJI_KAN, 37));
    CHECK(read_screen_line(1) == cat(KANJI_JI, TOUTEN, TOUTEN, KANJI_KAN));
}

TEST_CASE("c_roff moves a half-width word before a kinsoku character to the next line")
{
    // 行頭禁則文字の前の半角の単語は、単語ごと送る
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(repeat(KANJI_KAN, 37), "1500", TOUTEN, KANJI_KAN));
    CHECK(read_screen_line(0) == repeat(KANJI_KAN, 37));
    CHECK(read_screen_line(1) == cat("1500", TOUTEN, KANJI_KAN));
}

TEST_CASE("c_roff does not start a line with a space before a kinsoku character")
{
    // 行頭禁則文字の前が空白なら、空白の前の文字から送る
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(repeat(KANJI_KAN, 38), KANJI_JI, " ", TOUTEN, KANJI_KAN));
    CHECK(read_screen_line(0) == repeat(KANJI_KAN, 38));
    CHECK(read_screen_line(1) == cat(KANJI_JI, " ", TOUTEN, KANJI_KAN));
}

TEST_CASE("c_roff splits a half-width word before a kinsoku character when there is no word break")
{
    // 語の切れ目が無ければ、語の途中で 1 文字を送る
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(std::string(78, 'a'), TOUTEN));
    CHECK(read_screen_line(0) == std::string(77, 'a'));
    CHECK(read_screen_line(1) == cat("a", TOUTEN));
}

TEST_CASE("c_roff splits a half-width word before a kinsoku character if the word break is in the first half")
{
    // 語の切れ目が行の前半にしか無ければ、ほぼ 1 行分を送らずに語の途中で折り返す
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(" ", std::string(78, 'a'), TOUTEN));
    CHECK(read_screen_line(0) == cat(" ", std::string(77, 'a')));
    CHECK(read_screen_line(1) == cat("a", TOUTEN));
}

TEST_CASE("c_roff does not start a line with a kinsoku character after a space in the last column")
{
    // 最終桁に空白を書いた後に行頭禁則文字が来ても、空白の前の語から送る
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(repeat(KANJI_KAN, 39), "A ", TOUTEN, KANJI_KAN));
    CHECK(read_screen_line(0) == repeat(KANJI_KAN, 39));
    CHECK(read_screen_line(1) == cat("A ", TOUTEN, KANJI_KAN));
}

TEST_CASE("c_roff does not move so many cells that the kinsoku character reaches the last column")
{
    // 送ると行頭禁則文字が次の行の最終桁に達するときは、送らずに折り返す
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat("a", KANJI_KAN, repeat(TOUTEN, 38), " ", TOUTEN));
    CHECK(read_screen_line(0) == cat("a", KANJI_KAN, repeat(TOUTEN, 38)));
    CHECK(read_screen_line(1) == std::string(TOUTEN));
}

TEST_CASE("c_roff does not split a half-width word before a closing punctuation")
{
    // 語の途中で折り返すときは、半角の閉じ括弧の前では折り返さない
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat(KANJI_KAN, std::string(71, 'a'), "(150)", TOUTEN));
    CHECK(read_screen_line(0) == cat(KANJI_KAN, std::string(71, 'a'), "(15"));
    CHECK(read_screen_line(1) == cat("0)", TOUTEN));
}

TEST_CASE("c_roff does not leave a line of only spaces when the line starts with spaces")
{
    // 行頭の空白の直後しか語の切れ目が無ければ、空白だけの行を残さずに今の位置で折り返す
    ScopedInputTerm term;
    term_clear();
    c_roff(TERM_WHITE, cat("  ", KANJI_KAN, repeat(TOUTEN, 38)));
    CHECK(read_screen_line(0) == cat("  ", KANJI_KAN, repeat(TOUTEN, 37)));
    CHECK(read_screen_line(1) == std::string(TOUTEN));
}
#endif
