/*!
 * @brief アイテムの定義に共通するフラグ名の読み込み (grab_one_item_flag) のテスト
 *
 * 未知のフラグ名の表示は呼び出し側の各読み込み処理が行うため、表示の文言は
 * test-artifact-reader.cpp・test-baseitem-reader.cpp・test-ego-reader.cpp で扱う。
 */

#include "info-reader/item-flag-reader.h"
#include "object-enchant/tr-flags.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trg-types.h"
#include "util/flag-group.h"
#include "view/display-messages.h"
#include <doctest/doctest.h>

TEST_CASE("grab_one_item_flag sets a flag found in the item flag table")
{
    TrFlags flags{};
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags;
    ScopedMessageCapture diagnostics;
    CHECK(grab_one_item_flag(flags, gen_flags, "STR"));
    CHECK(flags.has(TR_STR));
    CHECK(flags.count() == 1);
    CHECK_FALSE(gen_flags.any());
    CHECK(diagnostics.messages().empty());
}

TEST_CASE("grab_one_item_flag sets a flag found in the generation flag table")
{
    TrFlags flags{};
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags;
    ScopedMessageCapture diagnostics;
    CHECK(grab_one_item_flag(flags, gen_flags, "INSTA_ART"));
    CHECK(gen_flags.has(ItemGenerationTraitType::INSTA_ART));
    CHECK(gen_flags.count() == 1);
    CHECK_FALSE(flags.any());
    CHECK(diagnostics.messages().empty());
}

TEST_CASE("grab_one_item_flag keeps the flags already set")
{
    TrFlags flags{ TR_DEX };
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags{ ItemGenerationTraitType::CURSED };
    CHECK(grab_one_item_flag(flags, gen_flags, "STR"));
    CHECK(grab_one_item_flag(flags, gen_flags, "INSTA_ART"));
    CHECK(flags.has(TR_DEX));
    CHECK(flags.has(TR_STR));
    CHECK(flags.count() == 2);
    CHECK(gen_flags.has(ItemGenerationTraitType::CURSED));
    CHECK(gen_flags.has(ItemGenerationTraitType::INSTA_ART));
    CHECK(gen_flags.count() == 2);
}

TEST_CASE("grab_one_item_flag rejects an unknown name without setting or printing anything")
{
    TrFlags flags{ TR_DEX };
    EnumClassFlagGroup<ItemGenerationTraitType> gen_flags{ ItemGenerationTraitType::CURSED };
    ScopedMessageCapture diagnostics;
    for (const auto *name : { "UNKNOWN_FLAG", "", "str", "STR ", "insta_art" }) {
        CAPTURE(name);
        CHECK_FALSE(grab_one_item_flag(flags, gen_flags, name));
    }
    CHECK(flags.has(TR_DEX));
    CHECK(flags.count() == 1);
    CHECK(gen_flags.has(ItemGenerationTraitType::CURSED));
    CHECK(gen_flags.count() == 1);
    CHECK(diagnostics.messages().empty());
}
