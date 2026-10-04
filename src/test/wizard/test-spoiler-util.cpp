/*!
 * @brief スポイラーの出力の共通処理のテスト
 *
 * 文字列を折り返しながらスポイラーのファイルへ書き出す spoil_out() を検証する。
 * spoil_out() は書き出し先をグローバルの spoiler_file で受け取り、行バッファを static に持つので、
 * 各テストでは一時ファイルに向けて書き出し、最後に flush_buffer を指定して行バッファを空にする。
 *
 * 2バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
 */

#include "wizard/spoiler-util.h"

#include "test/scoped-restore.h"
#include "test/string-helpers.h"
#include "util/finalizer.h"

#include <cstdio>
#include <doctest/doctest.h>
#include <initializer_list>
#include <string>
#include <string_view>

using namespace test;

namespace {
/*!
 * @brief 文字列を順に spoil_out() へ渡し、最後に flush_buffer を指定して書き出された内容を返す
 * @param texts spoil_out() へ渡す文字列の並び
 * @return 書き出された内容
 */
std::string capture_spoil_out(std::initializer_list<std::string_view> texts)
{
    auto *fp = std::tmpfile();
    REQUIRE(fp != nullptr);
    const auto close_file = util::make_finalizer([fp] { std::fclose(fp); });
    const auto restore = scoped_restore(spoiler_file);
    spoiler_file = fp;

    for (const auto text : texts) {
        spoil_out(text);
    }

    spoil_out({}, true);

    std::rewind(fp);
    std::string result;
    for (auto ch = std::fgetc(fp); ch != EOF; ch = std::fgetc(fp)) {
        result.push_back(static_cast<char>(ch));
    }

    return result;
}
}

TEST_CASE("spoil_out writes the buffered line followed by a blank line on flush")
{
    CHECK(capture_spoil_out({ "abc ", "def" }) == "abc def\n\n");
}

TEST_CASE("spoil_out writes only a newline when nothing is buffered")
{
    CHECK(capture_spoil_out({}) == "\n");
}

TEST_CASE("spoil_out removes trailing spaces on flush")
{
    CHECK(capture_spoil_out({ "abc   " }) == "abc\n\n");
}

TEST_CASE("spoil_out keeps a line of a single half-width character on flush")
{
    CHECK(capture_spoil_out({ "X" }) == "X\n\n");
    CHECK(capture_spoil_out({ "X  " }) == "X\n\n");
}

TEST_CASE("spoil_out writes a line break in the text as is")
{
    CHECK(capture_spoil_out({ "abc\ndef" }) == "abc\ndef\n\n");
}

TEST_CASE("spoil_out wraps a long text at a space")
{
    // 1行は75桁まで。折り返す位置の空白は書き出さない
    const auto expected = cat(repeat("abcd ", 14), "abcd\n", repeat("abcd ", 4), "abcd\n\n");
    CHECK(capture_spoil_out({ repeat("abcd ", 20) }) == expected);
}

#ifdef JP
TEST_CASE("spoil_out wraps a long Japanese text without splitting a 2-byte character")
{
    // 2バイト文字は1行に36文字 (72バイト) まで
    const auto expected = cat(repeat(KANJI_KAN, 36), "\n", repeat(KANJI_KAN, 14), "\n\n");
    CHECK(capture_spoil_out({ repeat(KANJI_KAN, 50) }) == expected);
}
#endif
