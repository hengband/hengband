/*!
 * @brief アイテムの記述の共通処理のテスト
 *
 * 銘を記述に使う形へ変換する get_inscription() のうち、未鑑定品の銘をそのまま書き出す経路を検証する。
 * 能力の略記を作る get_ability_abbreviation() は、ベースアイテムを 1 種類だけ用意して、祝福の書き方を検証する。
 *
 * 多バイト文字のテストデータは16進エスケープで書く (src/test/README.md を参照)。
 */

#include "flavor/flavor-util.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "test/scoped-vector-wrapper.h"
#include "test/string-helpers.h"
#include <doctest/doctest.h>
#include <string>
#include <string_view>

using namespace test;

namespace {

#ifdef JP
constexpr std::string_view KANJI_ZEN = "\xe5\x96\x84"; //!< 善
constexpr std::string_view KANJI_JA = "\xe9\x82\xaa"; //!< 邪
constexpr std::string_view KANJI_HI = "\xe7\x81\xab"; //!< 火
constexpr std::string_view KANJI_IKARI = "\xe6\x80\x92"; //!< 怒
constexpr std::string_view KANJI_SETSU = "\xe5\x88\x87"; //!< 切
#endif

/*!
 * @brief 特性を持たない剣のベースアイテムを 1 種類だけ用意し、そのアイテムを返す
 * @details 呼び出し側は、先に ScopedVectorWrapper<BaseitemList> でベースアイテムの一覧を退避しておく。
 */
ItemEntity make_plain_sword()
{
    auto &baseitems = BaseitemList::get_instance();
    baseitems.resize(2);
    auto &baseitem = baseitems.get_baseitem(1);
    baseitem.name = "Test";
    baseitem.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    ItemEntity item;
    item.bi_id = 1;
    item.bi_key = baseitem.bi_key;
    return item;
}

}

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

TEST_CASE("get_ability_abbreviation writes the blessing after its own separator")
{
    ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    auto item = make_plain_sword();

    SUBCASE("no separator is written for an item without a blessing")
    {
        item.art_flags.set(TR_AGGRAVATE);
        CHECK(get_ability_abbreviation(item, false, true) == "Ag");
    }

    SUBCASE("the blessing of the gods of good")
    {
        item.art_flags.set(TR_BLESSED);
        CHECK(get_ability_abbreviation(item, false, true) == "\"Good");
#ifdef JP
        CHECK(get_ability_abbreviation(item, true, true) == cat("\"", KANJI_ZEN));
#endif
    }

    SUBCASE("the blessing of the gods of evil")
    {
        item.art_flags.set(TR_BLESSED_EVIL);
        CHECK(get_ability_abbreviation(item, false, true) == "\"Evil");
#ifdef JP
        CHECK(get_ability_abbreviation(item, true, true) == cat("\"", KANJI_JA));
#endif
    }

    SUBCASE("the blessing comes right after the miscellaneous abilities")
    {
        // 耐性・その他・祝福・属性の刃・スレイの順。祝福の「邪」は、スレイの「邪」と区切りで見分ける
        item.art_flags.set({ TR_RES_FIRE, TR_AGGRAVATE, TR_BLESSED_EVIL, TR_VORPAL, TR_SLAY_EVIL });
        CHECK(get_ability_abbreviation(item, false, true) == "Fi;Ag\"Evil|Sl/*");
#ifdef JP
        CHECK(get_ability_abbreviation(item, true, true) == cat("r", KANJI_HI, ";", KANJI_IKARI, "\"", KANJI_JA, "|", KANJI_SETSU, "/", KANJI_JA));
#endif
    }
}

TEST_CASE("get_ability_abbreviation omits the blessing of the base item unless all abilities are requested")
{
    ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    auto item = make_plain_sword();
    BaseitemList::get_instance().get_baseitem(1).flags.set(TR_BLESSED);

    CHECK(get_ability_abbreviation(item, false, false).empty());
    CHECK(get_ability_abbreviation(item, false, true) == "\"Good");
}
