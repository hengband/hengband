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

#endif
