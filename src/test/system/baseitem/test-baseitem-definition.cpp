/*!
 * @brief ベースアイテム定義のテスト
 */

#include "artifact/random-art-effects.h"
#include "object-enchant/tr-types.h"
#include "system/baseitem/baseitem-definition.h"

#include <doctest/doctest.h>

#include <string>

TEST_CASE("BaseitemDefinition::clone_without_required_fields preserves only optional fields")
{
    BaseitemDefinition original;
    original.name = "Required name";
    original.init_symbol({ 7, '!' });
    original.bi_key = BaseitemKey(ItemKindType::POTION, 1);
    original.level = 12;
    original.weight = 13;
    original.cost = 14;
    original.text = "Flavor";
    original.flavor_name = "Unknown";
    original.pval = 1;
    original.ac = 2;
    original.damage_dice = Dice(3, 4);
    original.to_h = 5;
    original.to_d = 6;
    original.to_a = 7;
    for (size_t i = 0; i < original.alloc_tables.size(); ++i) {
        original.alloc_tables[i] = { static_cast<int>(i + 8), static_cast<short>(i + 9) };
    }
    original.act_idx = RandomArtActType::LIGHT;
    original.flags.set(TR_STR);
    original.gen_flags.set(ItemGenerationTraitType::INSTA_ART);
    original.easy_know = true;

    auto cloned = original.clone_without_required_fields();
    CHECK(cloned.name.empty());
    CHECK(cloned.get_symbol() == DisplaySymbol{});
    CHECK(cloned.bi_key == BaseitemKey{});
    CHECK(cloned.level == 0);
    CHECK(cloned.weight == 0);
    CHECK(cloned.cost == 0);
    CHECK(cloned.text == original.text);
    CHECK(cloned.flavor_name == original.flavor_name);
    CHECK(cloned.pval == original.pval);
    CHECK(cloned.ac == original.ac);
    CHECK(cloned.damage_dice == original.damage_dice);
    CHECK(cloned.to_h == original.to_h);
    CHECK(cloned.to_d == original.to_d);
    CHECK(cloned.to_a == original.to_a);
    for (size_t i = 0; i < original.alloc_tables.size(); ++i) {
        CHECK(cloned.alloc_tables[i].level == original.alloc_tables[i].level);
        CHECK(cloned.alloc_tables[i].chance == original.alloc_tables[i].chance);
    }
    CHECK(cloned.act_idx == original.act_idx);
    CHECK(cloned.flags == original.flags);
    CHECK(cloned.gen_flags == original.gen_flags);
    CHECK(cloned.easy_know == original.easy_know);

    cloned.text = "Changed";
    cloned.flavor_name = "Changed";
    cloned.alloc_tables[0] = { 20, 21 };
    cloned.flags.set(TR_DEX);
    cloned.gen_flags.clear();
    CHECK(original.name == "Required name");
    CHECK(original.get_symbol() == DisplaySymbol(7, '!'));
    CHECK(original.bi_key == BaseitemKey(ItemKindType::POTION, 1));
    CHECK(original.level == 12);
    CHECK(original.weight == 13);
    CHECK(original.cost == 14);
    CHECK(original.text == "Flavor");
    CHECK(original.flavor_name == "Unknown");
    CHECK(original.alloc_tables[0].level == 8);
    CHECK(original.alloc_tables[0].chance == 9);
    CHECK_FALSE(original.flags.has(TR_DEX));
    CHECK(original.gen_flags.has(ItemGenerationTraitType::INSTA_ART));
}

TEST_CASE("BaseitemDefinition::stripped_name removes trailing tilde and hash")
{
    BaseitemDefinition baseitem;

    baseitem.name = "Potion~";
    CHECK(baseitem.stripped_name() == "Potion ");

    baseitem.name = "Statue#";
    CHECK(baseitem.stripped_name() == "Statue ");
}

TEST_CASE("BaseitemDefinition::stripped_name removes both leading and trailing tilde and hash")
{
    BaseitemDefinition baseitem;

    baseitem.name = "#Statue#";
    CHECK(baseitem.stripped_name() == "Statue ");

    baseitem.name = "~Potion~";
    CHECK(baseitem.stripped_name() == "Potion ");
}

TEST_CASE("BaseitemDefinition::stripped_name keeps spaces between words")
{
    BaseitemDefinition baseitem;

    baseitem.name = "& Ration~ of Food";
    CHECK(baseitem.stripped_name() == "Ration of Food ");

    baseitem.name = "[Conjurings & Tricks]";
    CHECK(baseitem.stripped_name() == "[Conjurings & Tricks] ");
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
