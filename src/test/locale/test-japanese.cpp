/*!
 * @brief 日本語の処理のテスト
 *
 * 2バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
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
#ifdef SJIS
constexpr auto LINE_START_KINSOKU_CHARS = std::to_array<std::string_view>({
    "\x81\x41", // 、
    "\x81\x42", // 。
    "\x81\x43", // ，
    "\x81\x44", // ．
    "\x81\x48", // ？
    "\x81\x49", // ！
    "\x81\x45", // ・
    "\x81\x46", // ：
    "\x81\x47", // ；
    "\x83\x40", // ァ
    "\x83\x42", // ィ
    "\x83\x44", // ゥ
    "\x83\x46", // ェ
    "\x83\x48", // ォ
    "\x83\x83", // ャ
    "\x83\x85", // ュ
    "\x83\x87", // ョ
    "\x83\x62", // ッ
    "\x83\x8e", // ヮ
    "\x83\x95", // ヵ
    "\x83\x96", // ヶ
    "\x82\x9f", // ぁ
    "\x82\xa1", // ぃ
    "\x82\xa3", // ぅ
    "\x82\xa5", // ぇ
    "\x82\xa7", // ぉ
    "\x82\xe1", // ゃ
    "\x82\xe3", // ゅ
    "\x82\xe5", // ょ
    "\x82\xc1", // っ
    "\x82\xec", // ゎ
    "\x81\x5b", // ー
    "\x81\x60", // ～
    "\x81\x5d", // ‐
    "\x81\x54", // ゝ
    "\x81\x55", // ゞ
    "\x81\x52", // ヽ
    "\x81\x53", // ヾ
    "\x81\x58", // 々
    "\x81\x76", // 」
    "\x81\x78", // 』
    "\x81\x6a", // ）
    "\x81\x70", // ｝
    "\x81\x6e", // ］
    "\x81\x74", // 》
    "\x81\x7a", // 】
    "\x81\x6c", // 〕
    "\x81\x72", // 〉
    "\x81\x68", // ”
    "\x81\x66", // ’
});
//! 行頭禁則文字の一覧の終わり括弧に対応する始め括弧 (行頭禁則文字ではない)
constexpr auto OPENING_BRACKETS = std::to_array<std::string_view>({
    "\x81\x75", // 「
    "\x81\x77", // 『
    "\x81\x69", // （
    "\x81\x6f", // ｛
    "\x81\x6d", // ［
    "\x81\x73", // 《
    "\x81\x79", // 【
    "\x81\x6b", // 〔
    "\x81\x71", // 〈
    "\x81\x67", // “
    "\x81\x65", // ‘
});
#else
constexpr auto LINE_START_KINSOKU_CHARS = std::to_array<std::string_view>({
    "\xa1\xa2", // 、
    "\xa1\xa3", // 。
    "\xa1\xa4", // ，
    "\xa1\xa5", // ．
    "\xa1\xa9", // ？
    "\xa1\xaa", // ！
    "\xa1\xa6", // ・
    "\xa1\xa7", // ：
    "\xa1\xa8", // ；
    "\xa5\xa1", // ァ
    "\xa5\xa3", // ィ
    "\xa5\xa5", // ゥ
    "\xa5\xa7", // ェ
    "\xa5\xa9", // ォ
    "\xa5\xe3", // ャ
    "\xa5\xe5", // ュ
    "\xa5\xe7", // ョ
    "\xa5\xc3", // ッ
    "\xa5\xee", // ヮ
    "\xa5\xf5", // ヵ
    "\xa5\xf6", // ヶ
    "\xa4\xa1", // ぁ
    "\xa4\xa3", // ぃ
    "\xa4\xa5", // ぅ
    "\xa4\xa7", // ぇ
    "\xa4\xa9", // ぉ
    "\xa4\xe3", // ゃ
    "\xa4\xe5", // ゅ
    "\xa4\xe7", // ょ
    "\xa4\xc3", // っ
    "\xa4\xee", // ゎ
    "\xa1\xbc", // ー
    "\xa1\xc1", // ～
    "\xa1\xbe", // ‐
    "\xa1\xb5", // ゝ
    "\xa1\xb6", // ゞ
    "\xa1\xb3", // ヽ
    "\xa1\xb4", // ヾ
    "\xa1\xb9", // 々
    "\xa1\xd7", // 」
    "\xa1\xd9", // 』
    "\xa1\xcb", // ）
    "\xa1\xd1", // ｝
    "\xa1\xcf", // ］
    "\xa1\xd5", // 》
    "\xa1\xdb", // 】
    "\xa1\xcd", // 〕
    "\xa1\xd3", // 〉
    "\xa1\xc9", // ”
    "\xa1\xc7", // ’
});
//! 行頭禁則文字の一覧の終わり括弧に対応する始め括弧 (行頭禁則文字ではない)
constexpr auto OPENING_BRACKETS = std::to_array<std::string_view>({
    "\xa1\xd6", // 「
    "\xa1\xd8", // 『
    "\xa1\xca", // （
    "\xa1\xd0", // ｛
    "\xa1\xce", // ［
    "\xa1\xd4", // 《
    "\xa1\xda", // 【
    "\xa1\xcc", // 〔
    "\xa1\xd2", // 〈
    "\xa1\xc8", // “
    "\xa1\xc6", // ‘
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
    for (auto i = 0U; i < OPENING_BRACKETS.size(); i++) {
        CAPTURE(i);
        CHECK_FALSE(is_line_start_kinsoku(OPENING_BRACKETS[i]));
    }

    CHECK_FALSE(is_line_start_kinsoku("a"));
    CHECK_FALSE(is_line_start_kinsoku(LINE_START_KINSOKU_CHARS[0].substr(0, 1)));
}

#endif
