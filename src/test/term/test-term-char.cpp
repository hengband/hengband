/*!
 * @brief 画面の1マスに置かれる文字のテスト
 */

#include "term/term-char.h"

#include <doctest/doctest.h>

TEST_CASE("TermChar holds a single byte character")
{
    const auto ch = TermChar::from_byte('a');
    CHECK(ch.view() == "a");
    CHECK(ch.front() == 'a');
    CHECK_FALSE(ch.is_wide_lead());
    CHECK_FALSE(ch.is_wide_trail());

    // タイルの番号 (最上位ビットの立ったバイト) も1バイトの文字として持つ
    const auto tile = TermChar::from_byte('\x81');
    CHECK(tile.view() == "\x81");
    CHECK_FALSE(tile.is_wide_lead());

    // NUL も長さ1の文字として持つ
    CHECK(TermChar::from_byte('\0').view() == std::string_view("\0", 1));

    // 既定の文字は NUL
    CHECK(TermChar() == TermChar::from_byte('\0'));
}

TEST_CASE("TermChar holds the whole wide character in the lead cell")
{
    const auto ch = TermChar::wide("\xb0\xa1");
    CHECK(ch.view() == "\xb0\xa1");
    CHECK(ch.front() == '\xb0');
    CHECK(ch.is_wide_lead());
    CHECK_FALSE(ch.is_wide_trail());

    // 4バイトまで持ち、超える部分は捨てる
    CHECK(TermChar::wide("\xf0\x9f\x98\x80").view() == "\xf0\x9f\x98\x80");
    CHECK(TermChar::wide("\xf0\x9f\x98\x80\x80").view() == "\xf0\x9f\x98\x80");
}

TEST_CASE("TermChar of the trail cell of a wide character has no bytes")
{
    const auto ch = TermChar::wide_trail();
    CHECK(ch.view().empty());
    CHECK(ch.front() == '\0');
    CHECK(ch.is_wide_trail());
    CHECK_FALSE(ch.is_wide_lead());
}

TEST_CASE("TermChar compares the kind as well as the bytes")
{
    CHECK(TermChar::wide("\xb0\xa1") == TermChar::wide("\xb0\xa1"));
    CHECK(TermChar::wide("\xb0\xa1") != TermChar::wide("\xb0\xa2"));
    CHECK(TermChar::wide("a") != TermChar::from_byte('a'));
    CHECK(TermChar::wide_trail() != TermChar::wide(""));
}
