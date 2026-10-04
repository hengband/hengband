/*!
 * @brief アイテムの記述の共通処理のテスト
 *
 * 銘を記述に使う形へ変換する get_inscription() のうち、未鑑定品の銘をそのまま書き出す経路を検証する。
 * 鑑定済みの品の銘は「%」を能力の略記に置き換えるため、ベースアイテムなどの定義が要るので扱わない。
 *
 * 2バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
 */

#include "flavor/flavor-util.h"
#include "system/item/item-entity.h"
#include "test/string-helpers.h"
#include <doctest/doctest.h>
#include <string>

using namespace test;

TEST_CASE("get_inscription stops the inscription of an unidentified item at '#'")
{
    ItemEntity item;
    item.inscription = "abc#def";
    CHECK(get_inscription(item) == "abc");
}

#ifdef JP
TEST_CASE("get_inscription keeps 2-byte characters in the inscription of an unidentified item")
{
    ItemEntity item;
    item.inscription = cat("a", KANJI_KAN, KANJI_JI, "#def");
    CHECK(get_inscription(item) == cat("a", KANJI_KAN, KANJI_JI));
}

TEST_CASE("get_inscription does not read past the end of an inscription ending with a lead byte")
{
    // 前半バイトだけを書き出して終える
    const auto lead_byte = KANJI_KAN.substr(0, 1);
    ItemEntity item;
    item.inscription = cat("ab", lead_byte);
    CHECK(get_inscription(item) == cat("ab", lead_byte));
}
#endif
