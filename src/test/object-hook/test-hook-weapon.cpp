/*!
 * @brief 職業ごとの適正武器の判定 (object-hook/hook-weapon.cpp) のテスト
 *
 * プリーストの適正武器が、第一領域の善悪に合う祝福で決まることを確かめる。
 */

#include "object-enchant/tr-types.h"
#include "object-hook/hook-weapon.h"
#include "object/tval-types.h"
#include "player-info/class-types.h"
#include "player/player-realm.h"
#include "realm/realm-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/baseitem/baseitem-record.h"
#include "system/baseitem/baseitem-records.h"
#include "system/item/identification-flags.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "test/scoped-vector-wrapper.h"
#include <doctest/doctest.h>

namespace {

/*!
 * @brief 特性を持たないベースアイテムを 1 種類だけ用意し、*鑑定* 済みのそのアイテムを返す
 * @param tval 用意するベースアイテムの種類
 * @details 呼び出し側は、先に ScopedVectorWrapper でベースアイテムの一覧と鑑定の記録を退避しておく。
 */
ItemEntity make_fully_known_weapon(ItemKindType tval)
{
    auto &baseitems = BaseitemList::get_instance();
    baseitems.resize(2);
    auto &baseitem = baseitems.get_baseitem(1);
    baseitem.name = "Test";
    baseitem.bi_key = BaseitemKey(tval, 1);
    auto &records = BaseitemRecords::get_instance();
    records.initialize(2);
    records.get_record(1).mark_awareness(true);

    ItemEntity item;
    item.bi_id = 1;
    item.bi_key = baseitem.bi_key;
    item.mark_as_known();
    item.set_identification_flag(IdentificationFlag::FULL_KNOWN);
    return item;
}

}

TEST_CASE("object_is_favorite for a priest follows the blessing of the first realm")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    test::ScopedVectorWrapper<BaseitemRecords> saved_records{ BaseitemRecords::get_instance() };
    PlayerType player;
    player.pclass = PlayerClassType::PRIEST;
    PlayerRealm pr(&player);
    auto sword = make_fully_known_weapon(ItemKindType::SWORD);

    SUBCASE("a priest of a good realm favors a sword blessed by the gods of good")
    {
        pr.set(RealmType::LIFE);
        CHECK_FALSE(object_is_favorite(&player, &sword));

        sword.art_flags.set(TR_BLESSED_EVIL);
        CHECK_FALSE(object_is_favorite(&player, &sword));

        // 自らの神の祝福だけを持つ剣
        sword.art_flags.clear();
        sword.art_flags.set(TR_BLESSED);
        CHECK(object_is_favorite(&player, &sword));

        // もう一方の神の祝福も持つ剣は、自らの神の祝福があってもふさわしくない
        sword.art_flags.set(TR_BLESSED_EVIL);
        CHECK_FALSE(object_is_favorite(&player, &sword));
    }

    SUBCASE("a priest of an evil realm favors a sword blessed by the gods of evil")
    {
        pr.set(RealmType::DEATH);
        CHECK_FALSE(object_is_favorite(&player, &sword));

        sword.art_flags.set(TR_BLESSED);
        CHECK_FALSE(object_is_favorite(&player, &sword));

        // 自らの神の祝福だけを持つ剣
        sword.art_flags.clear();
        sword.art_flags.set(TR_BLESSED_EVIL);
        CHECK(object_is_favorite(&player, &sword));

        // もう一方の神の祝福も持つ剣は、自らの神の祝福があってもふさわしくない
        sword.art_flags.set(TR_BLESSED);
        CHECK_FALSE(object_is_favorite(&player, &sword));
    }

    SUBCASE("a blessing the player does not know yet does not count")
    {
        // *鑑定* していないアイテムの個別の特性は、プレイヤーにはまだ分からない
        pr.set(RealmType::DEATH);
        sword.art_flags.set(TR_BLESSED_EVIL);
        sword.reset_identification_flag(IdentificationFlag::FULL_KNOWN);
        CHECK_FALSE(object_is_favorite(&player, &sword));
    }
}

TEST_CASE("object_is_favorite for a priest favors a hafted weapon unless the other gods blessed it")
{
    test::ScopedVectorWrapper<BaseitemList> saved_baseitems{ BaseitemList::get_instance() };
    test::ScopedVectorWrapper<BaseitemRecords> saved_records{ BaseitemRecords::get_instance() };
    PlayerType player;
    player.pclass = PlayerClassType::PRIEST;
    PlayerRealm pr(&player);
    auto mace = make_fully_known_weapon(ItemKindType::HAFTED);

    SUBCASE("a hafted weapon without a blessing is always favored")
    {
        pr.set(RealmType::LIFE);
        CHECK(object_is_favorite(&player, &mace));
        pr.set(RealmType::DEATH);
        CHECK(object_is_favorite(&player, &mace));
    }

    SUBCASE("a hafted weapon blessed by the gods of good is not favored by a priest of an evil realm")
    {
        mace.art_flags.set(TR_BLESSED);
        pr.set(RealmType::LIFE);
        CHECK(object_is_favorite(&player, &mace));
        pr.set(RealmType::DEATH);
        CHECK_FALSE(object_is_favorite(&player, &mace));
    }

    SUBCASE("a hafted weapon blessed by the gods of evil is not favored by a priest of a good realm")
    {
        mace.art_flags.set(TR_BLESSED_EVIL);
        pr.set(RealmType::LIFE);
        CHECK_FALSE(object_is_favorite(&player, &mace));
        pr.set(RealmType::DEATH);
        CHECK(object_is_favorite(&player, &mace));
    }

    SUBCASE("a blessing of the other gods the player does not know yet does not count")
    {
        // *鑑定* していないアイテムの個別の特性は、プレイヤーにはまだ分からない
        mace.art_flags.set(TR_BLESSED);
        mace.reset_identification_flag(IdentificationFlag::FULL_KNOWN);
        pr.set(RealmType::DEATH);
        CHECK(object_is_favorite(&player, &mace));
    }
}
