#include "object-hook/hook-magic.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-realm.h"
#include "system/baseitem/baseitem-key.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "util/enum-converter.h"

/*!
 * @brief オブジェクトをプレイヤーが簡易使用コマンドで利用できるかを判定する /
 * Hook to determine if an object is useable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 利用可能ならばTRUEを返す
 */
bool item_tester_hook_use(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    const auto tval = o_ptr->bi_key.tval();
    if (tval == player_ptr->tval_ammo) {
        return true;
    }

    switch (tval) {
    case ItemKindType::SPIKE:
    case ItemKindType::STAFF:
    case ItemKindType::WAND:
    case ItemKindType::ROD:
    case ItemKindType::SCROLL:
    case ItemKindType::POTION:
    case ItemKindType::FOOD:
        return true;
    default:
        if (!o_ptr->is_known()) {
            return false;
        }

        for (int i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
            if ((&player_ptr->inventory_list[i] == o_ptr) && o_ptr->get_flags().has(TR_ACTIVATE)) {
                return true;
            }
        }

        return false;
    }
}

/*!
 * @brief オブジェクトがプレイヤーが使用可能な魔道書かどうかを判定する
 * @param o_ptr 判定したいオブ会ジェクトの構造体参照ポインタ
 * @return 学習できる魔道書ならばTRUEを返す
 */
bool item_tester_learn_spell(PlayerType *player_ptr, const ItemEntity *o_ptr)
{
    if (!o_ptr->is_spell_book()) {
        return false;
    }

    PlayerRealm pr(player_ptr);
    auto choices = PlayerRealm::get_realm2_choices(player_ptr->pclass);
    PlayerClass pc(player_ptr);
    if (pc.equals(PlayerClassType::PRIEST)) {
        if (PlayerRealm(player_ptr).realm1().is_good_attribute()) {
            choices.reset({ RealmType::DEATH, RealmType::DAEMON });
        } else {
            choices.reset({ RealmType::LIFE, RealmType::CRUSADE });
        }
    }

    const auto tval = o_ptr->bi_key.tval();
    if ((tval == ItemKindType::MUSIC_BOOK) && pc.equals(PlayerClassType::BARD)) {
        return true;
    }

    const auto book_realm = PlayerRealm::get_realm_of_book(tval);
    if (!PlayerRealm::is_magic(book_realm)) {
        return false;
    }

    return pr.realm1().equals(book_realm) || pr.realm2().equals(book_realm) || choices.has(book_realm);
}
