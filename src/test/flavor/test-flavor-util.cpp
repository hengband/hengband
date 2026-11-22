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
    // 後続のバイトが欠けた前半バイトは書き出さない
    const auto lead_byte = KANJI_KAN.substr(0, 1);
    ItemEntity item;
    item.inscription = cat("ab", lead_byte);
    CHECK(get_inscription(item) == "ab");
}

TEST_CASE("get_inscription cuts the inscription of an unidentified item by display width")
{
    // 全角文字は 2 桁なので、MAX_INSCRIPTION (76) 桁に入るのは 38 文字
    std::string inscription;
    std::string expected;
    for (auto i = 0; i < 40; i++) {
        inscription.append(KANJI_KAN);
        if (i < 38) {
            expected.append(KANJI_KAN);
        }
    }

    ItemEntity item;
    item.inscription = inscription;
    CHECK(get_inscription(item) == expected);
}

TEST_CASE("get_inscription does not add a 2-byte character exceeding the display width limit")
{
    // 半角 1 文字と全角 37 文字で 75 桁になり、次の全角文字は 77 桁になるので足さない
    std::string inscription("a");
    std::string expected("a");
    for (auto i = 0; i < 38; i++) {
        inscription.append(KANJI_KAN);
        if (i < 37) {
            expected.append(KANJI_KAN);
        }
    }

    ItemEntity item;
    item.inscription = inscription;
    CHECK(get_inscription(item) == expected);
}
#endif
