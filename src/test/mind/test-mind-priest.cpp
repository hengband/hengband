/*!
 * @brief プリーストの処理 (mind/mind-priest.cpp) のテスト
 *
 * プリーストにふさわしい祝福が第一領域の善悪で決まることと、武器に祝福を与える判定を確かめる。
 */

#include "mind/mind-priest.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/tr-types.h"
#include "object-enchant/trc-types.h"
#include "object/tval-types.h"
#include "player/player-realm.h"
#include "realm/realm-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "test/scoped-ego-list.h"
#include "test/scoped-rng.h"
#include "test/scoped-vector-wrapper.h"
#include <doctest/doctest.h>

namespace {

/*!
 * @brief 特性を持たない剣のベースアイテムを 1 種類だけ用意し、修正値 (+3,+3) のそのアイテムを返す
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
    item.to_h = 3;
    item.to_d = 3;
    return item;
}

}

TEST_CASE("get_priest_blessing follows the alignment of the first realm")
{
    PlayerType player;
    PlayerRealm pr(&player);

    // 善の領域 (生命・破邪) は善の神の祝福
    pr.set(RealmType::LIFE);
    CHECK(get_priest_blessing(&player) == TR_BLESSED);
    pr.set(RealmType::CRUSADE);
    CHECK(get_priest_blessing(&player) == TR_BLESSED);

    // 悪の領域 (暗黒・悪魔) は邪の神の祝福
    pr.set(RealmType::DEATH);
    CHECK(get_priest_blessing(&player) == TR_BLESSED_EVIL);
    pr.set(RealmType::DAEMON);
    CHECK(get_priest_blessing(&player) == TR_BLESSED_EVIL);
}

TEST_CASE("get_priest_blessing ignores the second realm")
{
    PlayerType player;
    PlayerRealm pr(&player);

    pr.set(RealmType::LIFE, RealmType::SORCERY);
    CHECK(get_priest_blessing(&player) == TR_BLESSED);
    pr.set(RealmType::DEATH, RealmType::SORCERY);
    CHECK(get_priest_blessing(&player) == TR_BLESSED_EVIL);
}

TEST_CASE("apply_blessing_to_weapon always blesses a weapon that is neither an artifact nor an ego")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    auto item = make_plain_sword();

    const auto outcome = apply_blessing_to_weapon(item);
    CHECK(outcome.result == BlessWeaponResult::BLESSED);
    CHECK_FALSE(outcome.is_curse_removed);
    CHECK_FALSE(outcome.is_disenchanted);
    CHECK(item.art_flags.has(TR_BLESSED));
    CHECK(item.discount == 99);
    CHECK(item.to_h == 3);
    CHECK(item.to_d == 3);
}

TEST_CASE("apply_blessing_to_weapon leaves a blessed weapon as it is")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    auto item = make_plain_sword();
    item.art_flags.set(TR_BLESSED);

    const auto outcome = apply_blessing_to_weapon(item);
    CHECK(outcome.result == BlessWeaponResult::ALREADY_BLESSED);
    CHECK(item.discount == 0);
}

TEST_CASE("apply_blessing_to_weapon removes a light curse and then blesses the weapon")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    auto item = make_plain_sword();
    item.curse_flags.set(CurseTraitType::CURSED);

    const auto outcome = apply_blessing_to_weapon(item);
    CHECK(outcome.result == BlessWeaponResult::BLESSED);
    CHECK(outcome.is_curse_removed);
    CHECK_FALSE(item.is_cursed());
    CHECK(item.art_flags.has(TR_BLESSED));
}

TEST_CASE("apply_blessing_to_weapon is disturbed by a curse that cannot be removed")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    for (const auto curse : { CurseTraitType::PERMA_CURSE, CurseTraitType::PERSISTENT_CURSE }) {
        CAPTURE(static_cast<int>(curse));
        auto item = make_plain_sword();
        item.curse_flags.set({ CurseTraitType::CURSED, curse });

        const auto outcome = apply_blessing_to_weapon(item);
        CHECK(outcome.result == BlessWeaponResult::DISTURBED);
        CHECK_FALSE(outcome.is_curse_removed);
        CHECK(item.curse_flags.has(curse));
        CHECK(item.art_flags.has_not(TR_BLESSED));
    }
}

TEST_CASE("apply_blessing_to_weapon is resisted by an ego weapon two times out of three")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    test::ScopedEgoList saved_egos;
    egos_info.try_emplace(EgoType::A_MORGUL);

    constexpr auto trials = 300U;
    auto blessed_count = 0;
    for (auto seed = 1U; seed <= trials; seed++) {
        const auto restore_rng = test::scoped_rng(seed);
        auto item = make_plain_sword();
        item.ego_idx = EgoType::A_MORGUL;

        const auto outcome = apply_blessing_to_weapon(item);
        CAPTURE(seed);
        if (outcome.result == BlessWeaponResult::BLESSED) {
            blessed_count++;
            CHECK(item.art_flags.has(TR_BLESSED));
            CHECK(item.to_h == 3);
            CHECK(item.to_d == 3);
            continue;
        }

        // 抵抗されると祝福は付かず、正の修正値が 1 ずつ下がる
        REQUIRE(outcome.result == BlessWeaponResult::RESISTED);
        CHECK(outcome.is_disenchanted);
        CHECK(item.art_flags.has_not(TR_BLESSED));
        CHECK(item.to_h == 2);
        CHECK(item.to_d == 2);
        CHECK(item.discount == 0);
    }

    // シードを固定しているので回数は毎回同じ。3 分の 1 (100 回) の前後であることだけを確かめる
    CHECK(blessed_count > 70);
    CHECK(blessed_count < 130);
}
