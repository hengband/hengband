/*!
 * @brief 日本語の処理のテスト
 *
 * 多バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
 */

#include "locale/japanese.h"

#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <array>
#include <string_view>

using namespace test;

#ifdef JP

namespace {
//! 行頭禁則文字の一覧のすべての文字
#ifdef UTF8
constexpr auto LINE_START_KINSOKU_CHARS = std::to_array<std::string_view>({
    "\xe3\x80\x81", // 、
    "\xe3\x80\x82", // 。
    "\xef\xbc\x8c", // ，
    "\xef\xbc\x8e", // ．
    "\xef\xbc\x9f", // ？
    "\xef\xbc\x81", // ！
    "\xe3\x83\xbb", // ・
    "\xef\xbc\x9a", // ：
    "\xef\xbc\x9b", // ；
    "\xe3\x82\xa1", // ァ
    "\xe3\x82\xa3", // ィ
    "\xe3\x82\xa5", // ゥ
    "\xe3\x82\xa7", // ェ
    "\xe3\x82\xa9", // ォ
    "\xe3\x83\xa3", // ャ
    "\xe3\x83\xa5", // ュ
    "\xe3\x83\xa7", // ョ
    "\xe3\x83\x83", // ッ
    "\xe3\x83\xae", // ヮ
    "\xe3\x83\xb5", // ヵ
    "\xe3\x83\xb6", // ヶ
    "\xe3\x81\x81", // ぁ
    "\xe3\x81\x83", // ぃ
    "\xe3\x81\x85", // ぅ
    "\xe3\x81\x87", // ぇ
    "\xe3\x81\x89", // ぉ
    "\xe3\x82\x83", // ゃ
    "\xe3\x82\x85", // ゅ
    "\xe3\x82\x87", // ょ
    "\xe3\x81\xa3", // っ
    "\xe3\x82\x8e", // ゎ
    "\xe3\x83\xbc", // ー
    "\xef\xbd\x9e", // ～
    "\xe2\x80\x90", // ‐
    "\xe3\x82\x9d", // ゝ
    "\xe3\x82\x9e", // ゞ
    "\xe3\x83\xbd", // ヽ
    "\xe3\x83\xbe", // ヾ
    "\xe3\x80\x85", // 々
    "\xe3\x80\x8d", // 」
    "\xe3\x80\x8f", // 』
    "\xef\xbc\x89", // ）
    "\xef\xbd\x9d", // ｝
    "\xef\xbc\xbd", // ］
    "\xe3\x80\x8b", // 》
    "\xe3\x80\x91", // 】
    "\xe3\x80\x95", // 〕
    "\xe3\x80\x89", // 〉
    "\xe2\x80\x9d", // ”
    "\xe2\x80\x99", // ’
});
//! 行末禁則文字の一覧のすべての文字 (行頭禁則文字の一覧の終わり括弧に対応する始め括弧と、半角の始め括弧)
constexpr auto LINE_END_KINSOKU_CHARS = std::to_array<std::string_view>({
    "\xe3\x80\x8c", // 「
    "\xe3\x80\x8e", // 『
    "\xef\xbc\x88", // （
    "\xef\xbd\x9b", // ｛
    "\xef\xbc\xbb", // ［
    "\xe3\x80\x8a", // 《
    "\xe3\x80\x90", // 【
    "\xe3\x80\x94", // 〔
    "\xe3\x80\x88", // 〈
    "\xe2\x80\x9c", // “
    "\xe2\x80\x98", // ‘
    "(",
    "[",
    "{",
});
#endif
}

TEST_CASE("is_line_start_kinsoku returns true for characters prohibited at the head of a line")
{
    for (auto i = 0U; i < LINE_START_KINSOKU_CHARS.size(); i++) {
        CAPTURE(i);
        CHECK(is_line_start_kinsoku(LINE_START_KINSOKU_CHARS[i]));
    }
}

TEST_CASE("is_line_start_kinsoku returns false for other characters")
{
    CHECK_FALSE(is_line_start_kinsoku(KANJI_KAN));
    for (auto i = 0U; i < LINE_END_KINSOKU_CHARS.size(); i++) {
        CAPTURE(i);
        CHECK_FALSE(is_line_start_kinsoku(LINE_END_KINSOKU_CHARS[i]));
    }

    CHECK_FALSE(is_line_start_kinsoku("a"));
    CHECK_FALSE(is_line_start_kinsoku(LINE_START_KINSOKU_CHARS[0].substr(0, 1)));
}

TEST_CASE("is_line_end_kinsoku returns true for characters prohibited at the end of a line")
{
    for (auto i = 0U; i < LINE_END_KINSOKU_CHARS.size(); i++) {
        CAPTURE(i);
        CHECK(is_line_end_kinsoku(LINE_END_KINSOKU_CHARS[i]));
    }
}

TEST_CASE("is_line_end_kinsoku returns false for other characters")
{
    CHECK_FALSE(is_line_end_kinsoku(KANJI_KAN));
    for (auto i = 0U; i < LINE_START_KINSOKU_CHARS.size(); i++) {
        CAPTURE(i);
        CHECK_FALSE(is_line_end_kinsoku(LINE_START_KINSOKU_CHARS[i]));
    }

    CHECK_FALSE(is_line_end_kinsoku("a"));
    CHECK_FALSE(is_line_end_kinsoku(")"));
}

#endif
