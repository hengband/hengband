/*!
 * @brief ベースアイテム定義のテスト
 */

#include "system/baseitem/baseitem-definition.h"

#include <doctest/doctest.h>

#include <string>

TEST_CASE("BaseitemDefinition::stripped_name removes trailing tilde and hash")
{
    BaseitemDefinition baseitem;

    baseitem.name = "Potion~";
    CHECK(baseitem.stripped_name() == "Potion ");

    baseitem.name = "Statue#";
    CHECK(baseitem.stripped_name() == "Statue ");
}

#ifdef JP
TEST_CASE("BaseitemDefinition::stripped_name removes a hash following a multibyte character")
{
    /*
     * 日本語版のビルドでは文字列リテラルの文字コードが変換されるため、2バイト文字はエスケープで書く。
     * 「ル」は後半バイトも2バイト文字の先頭バイトと同じ範囲の値をとる。
     */
#ifdef SJIS
    const std::string ru = "\x83\x8b"; // ル (Shift_JIS)
#else
    const std::string ru = "\xa5\xeb"; // ル (EUC-JP)
#endif
    BaseitemDefinition baseitem;
    baseitem.name = ru + "#";
    CHECK(baseitem.stripped_name() == ru + " ");
}

#ifdef SJIS
TEST_CASE("BaseitemDefinition::stripped_name keeps a trail byte equal to tilde")
{
    BaseitemDefinition baseitem;
    baseitem.name = "\x83\x7e"; // ミ (Shift_JIS、後半バイトが '~')
    CHECK(baseitem.stripped_name() == "\x83\x7e ");
}
#endif
#endif
