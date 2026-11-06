#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object/tval-types.h"
#include "sv-definition/sv-armor-types.h"
#include "sv-definition/sv-lite-types.h"
#include "sv-definition/sv-protector-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/item/item-entity.h"
#include "test/scoped-ego-list.h"
#include "test/scoped-rng.h"
#include "util/enum-converter.h"
#include <array>
#include <doctest/doctest.h>
#include <limits>
#include <utility>

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("apply_ego skips probability rolls at zero and certain percent")
{
    for (const auto chance : { 0, 100 }) {
        test::ScopedEgoList saved_egos;
        auto &ego = egos_info.try_emplace(EgoType::A_MORGUL).first->second;
        ego.cost = 12345;
        const auto rng_guard = test::scoped_rng();
        ego_generate_type extra;
        extra.chance = chance;
        extra.tr_flags = { TR_RES_FIRE };
        ego.xtra_flags.push_back(std::move(extra));
        ItemEntity item;
        item.ego_idx = i2enum<EgoType>(4);
        item.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
        auto expected_rng = get_game_rng();
        apply_ego(&item, 10);
        CHECK(item.art_flags.has(TR_RES_FIRE) == (chance == 100));
        const auto expected = expected_rng();
        const auto actual = get_game_rng()();
        CHECK(actual == expected);
    }
}

/*!
 * @brief 対象モジュールの数値境界と通常値の計算結果を確認する
 */
TEST_CASE("apply_ego saturates composed bonuses to save storage")
{
    test::ScopedEgoList saved_egos;
    auto &ego = egos_info.try_emplace(EgoType::A_MORGUL).first->second;
    ego.cost = 12345;
    const auto rng_guard = test::scoped_rng();
    ego.base_to_h = 32767;
    ego.base_to_d = 32767;
    ego.base_to_a = -32768;
    ego.max_pval = -32768;
    ItemEntity item;
    item.ego_idx = i2enum<EgoType>(4);
    item.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    item.to_h = 1;
    item.to_d = std::numeric_limits<int>::max();
    item.to_a = -1;
    item.pval = -32768;
    apply_ego(&item, 10);
    CHECK(item.to_h == 32767);
    CHECK(item.to_d == 32767);
    CHECK(item.to_a == -32768);
    CHECK(item.pval == -32768);
}

/*!
 * @brief ウィザード操作で荒野の大きな基本レベルを渡しても追加攻撃の乱数引数を保つ
 */
TEST_CASE("apply_ego bounds extra attacks at wizard wilderness generation levels")
{
    test::ScopedEgoList saved_egos;
    auto &ego = egos_info.try_emplace(EgoType::ATTACKS).first->second;
    ego.cost = 1;
    ego.max_pval = 32767;
    ego.flags.set(TR_BLOWS);
    const auto rng_guard = test::scoped_rng();
    ItemEntity item;
    item.ego_idx = EgoType::ATTACKS;
    item.bi_key = BaseitemKey(ItemKindType::SWORD, 1);
    apply_ego(&item, std::numeric_limits<DEPTH>::max());
    CHECK(item.pval == 3);
}

TEST_CASE("is_worthless_ego detects zero rating and each curse flag")
{
    EgoItemDefinition ego;
    CHECK(is_worthless_ego(ego));
    ego.rating = 10;
    CHECK(!is_worthless_ego(ego));
    for (const auto curse : { ItemGenerationTraitType::CURSED, ItemGenerationTraitType::HEAVY_CURSE, ItemGenerationTraitType::PERMA_CURSE }) {
        ego.gen_flags.clear();
        ego.gen_flags.set(curse);
        CHECK(is_worthless_ego(ego));
    }
}

TEST_CASE("can_apply_ego checks slots, kind exclusions and undefined IDs")
{
    test::ScopedEgoList saved_egos;
    const auto add = [](EgoType id, INVENTORY_IDX slot) {
        auto &ego = egos_info.try_emplace(id).first->second;
        ego.idx = id;
        ego.rating = 10;
        ego.slot = slot;
    };
    for (const auto id : { EgoType::DF, EgoType::SHARPNESS, EgoType::EARTHQUAKES, EgoType::DIGGING, EgoType::MORGUL }) {
        add(id, INVEN_MAIN_HAND);
    }
    egos_info.at(EgoType::MORGUL).rating = 0;
    for (const auto id : { EgoType::RESISTANCE, EgoType::DWARVEN, EgoType::DRUID, EgoType::PERMANENCE, EgoType::TWILIGHT }) {
        add(id, INVEN_BODY);
    }
    for (const auto id : { EgoType::REFLECTION, EgoType::S_DWARVEN, EgoType::ENDURE_FIRE }) {
        add(id, INVEN_SUB_HAND);
    }
    for (const auto id : { EgoType::TELEPATHY, EgoType::INFRAVISION, EgoType::SEEING }) {
        add(id, INVEN_HEAD);
    }
    add(EgoType::LITE_LONG, INVEN_LITE);
    add(EgoType::FREE_ACTION, INVEN_ARMS);
    add(EgoType::QUIET, INVEN_FEET);
    add(EgoType::ACCURACY, INVEN_BOW);
    add(EgoType::PROTECTION, INVEN_OUTER);
    // 名前のないIDも定義が存在すれば有効。
    add(static_cast<EgoType>(114), INVEN_AMMO);
    add(static_cast<EgoType>(241), INVEN_ARMS);
    struct Compatibility {
        ItemKindType kind;
        int sval;
        EgoType ego;
        bool allowed;
    };
    const std::array cases{
        Compatibility{ ItemKindType::SWORD, SV_LONG_SWORD, EgoType::DF, true },
        Compatibility{ ItemKindType::SWORD, SV_LONG_SWORD, EgoType::EARTHQUAKES, false },
        Compatibility{ ItemKindType::SWORD, SV_POISON_NEEDLE, EgoType::DF, false },
        Compatibility{ ItemKindType::SWORD, SV_DIAMOND_EDGE, EgoType::DF, false },
        Compatibility{ ItemKindType::HAFTED, SV_WAR_HAMMER, EgoType::SHARPNESS, false },
        Compatibility{ ItemKindType::HAFTED, SV_WAR_HAMMER, EgoType::EARTHQUAKES, true },
        Compatibility{ ItemKindType::POLEARM, SV_SPEAR, EgoType::SHARPNESS, false },
        Compatibility{ ItemKindType::POLEARM, SV_SPEAR, EgoType::EARTHQUAKES, false },
        Compatibility{ ItemKindType::POLEARM, SV_DEATH_SCYTHE, EgoType::DF, false },
        Compatibility{ ItemKindType::SWORD, SV_LONG_SWORD, EgoType::DIGGING, false },
        Compatibility{ ItemKindType::HAFTED, SV_WAR_HAMMER, EgoType::DIGGING, false },
        Compatibility{ ItemKindType::POLEARM, SV_SPEAR, EgoType::DIGGING, false },
        Compatibility{ ItemKindType::HELM, SV_HARD_LEATHER_CAP, EgoType::INFRAVISION, true },
        Compatibility{ ItemKindType::HELM, SV_HARD_LEATHER_CAP, EgoType::SEEING, true },
        Compatibility{ ItemKindType::HELM, SV_HARD_LEATHER_CAP, EgoType::TELEPATHY, false },
        Compatibility{ ItemKindType::CROWN, SV_IRON_CROWN, EgoType::TELEPATHY, true },
        Compatibility{ ItemKindType::CROWN, SV_IRON_CROWN, EgoType::SEEING, true },
        Compatibility{ ItemKindType::CROWN, SV_IRON_CROWN, EgoType::INFRAVISION, false },
        Compatibility{ ItemKindType::SHIELD, SV_MIRROR_SHIELD, EgoType::REFLECTION, false },
        Compatibility{ ItemKindType::SHIELD, SV_SMALL_METAL_SHIELD, EgoType::S_DWARVEN, true },
        Compatibility{ ItemKindType::SHIELD, SV_LARGE_METAL_SHIELD, EgoType::S_DWARVEN, true },
        Compatibility{ ItemKindType::SHIELD, SV_SMALL_LEATHER_SHIELD, EgoType::S_DWARVEN, false },
        Compatibility{ ItemKindType::SHIELD, SV_MIRROR_SHIELD, EgoType::ENDURE_FIRE, true },
        Compatibility{ ItemKindType::SOFT_ARMOR, SV_ROBE, EgoType::TWILIGHT, true },
        Compatibility{ ItemKindType::SOFT_ARMOR, SV_ROBE, EgoType::PERMANENCE, true },
        Compatibility{ ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR, EgoType::TWILIGHT, false },
        Compatibility{ ItemKindType::SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR, EgoType::DRUID, true },
        Compatibility{ ItemKindType::SOFT_ARMOR, SV_ROBE, EgoType::DWARVEN, false },
        Compatibility{ ItemKindType::HARD_ARMOR, SV_CHAIN_MAIL, EgoType::DWARVEN, true },
        Compatibility{ ItemKindType::HARD_ARMOR, SV_CHAIN_MAIL, EgoType::DRUID, false },
        Compatibility{ ItemKindType::HARD_ARMOR, SV_CHAIN_MAIL, EgoType::PERMANENCE, false },
        Compatibility{ ItemKindType::DIGGING, 1, EgoType::DIGGING, true },
        Compatibility{ ItemKindType::DIGGING, 1, EgoType::DF, false },
        Compatibility{ ItemKindType::LITE, SV_LITE_FEANOR, EgoType::LITE_LONG, false },
        Compatibility{ ItemKindType::LITE, SV_LITE_LANTERN, EgoType::LITE_LONG, true },
        Compatibility{ ItemKindType::SHIELD, SV_DRAGON_SHIELD, EgoType::ENDURE_FIRE, false },
        Compatibility{ ItemKindType::HELM, SV_DRAGON_HELM, EgoType::INFRAVISION, false },
        Compatibility{ ItemKindType::BOOTS, SV_PAIR_OF_DRAGON_GREAVE, EgoType::QUIET, false },
        Compatibility{ ItemKindType::GLOVES, SV_SET_OF_DRAGON_GLOVES, EgoType::FREE_ACTION, false },
        Compatibility{ ItemKindType::DRAG_ARMOR, 1, EgoType::RESISTANCE, false },
        Compatibility{ ItemKindType::SWORD, SV_LONG_SWORD, EgoType::MORGUL, false },
        Compatibility{ ItemKindType::POTION, 1, EgoType::DF, false },
        Compatibility{ ItemKindType::RING, 1, EgoType::DF, false },
        Compatibility{ ItemKindType::AMULET, 1, EgoType::DF, false },
        Compatibility{ ItemKindType::SWORD, SV_LONG_SWORD, static_cast<EgoType>(1), false },
        Compatibility{ ItemKindType::SWORD, SV_LONG_SWORD, EgoType::NONE, false },
        Compatibility{ ItemKindType::ARROW, 1, static_cast<EgoType>(114), true },
        Compatibility{ ItemKindType::SHOT, 1, static_cast<EgoType>(114), true },
        Compatibility{ ItemKindType::BOLT, 1, static_cast<EgoType>(114), true },
        Compatibility{ ItemKindType::GLOVES, SV_SET_OF_LEATHER_GLOVES, static_cast<EgoType>(241), true },
        Compatibility{ ItemKindType::BOW, 1, EgoType::ACCURACY, true },
        Compatibility{ ItemKindType::CLOAK, 1, EgoType::PROTECTION, true },
        Compatibility{ ItemKindType::SWORD, SV_LONG_SWORD, EgoType::TELEPATHY, false },
    };
    const auto size_before = egos_info.size();
    for (const auto &entry : cases) {
        CAPTURE(static_cast<int>(entry.kind));
        CAPTURE(entry.sval);
        CAPTURE(static_cast<int>(entry.ego));
        CHECK(can_apply_ego(BaseitemKey(entry.kind, entry.sval), entry.ego) == entry.allowed);
    }
    CHECK(egos_info.size() == size_before);
    for (const auto curse : { ItemGenerationTraitType::CURSED, ItemGenerationTraitType::HEAVY_CURSE, ItemGenerationTraitType::PERMA_CURSE }) {
        auto &ego = egos_info.at(EgoType::DF);
        ego.gen_flags.clear();
        ego.gen_flags.set(curse);
        CHECK(!can_apply_ego(BaseitemKey(ItemKindType::SWORD, SV_LONG_SWORD), EgoType::DF));
    }
}
