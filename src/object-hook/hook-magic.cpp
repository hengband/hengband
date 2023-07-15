#include "object-hook/hook-magic.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-info/class-info.h"
#include "player/player-realm.h"
#include "realm/realm-names-table.h"
#include "system/baseitem-info.h"
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
            if (&player_ptr->inventory_list[i] == o_ptr) {
                auto flags = object_flags(o_ptr);
                if (flags.has(TR_ACTIVATE)) {
                    return true;
                }
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

    int32_t choices = realm_choices2[enum2i(player_ptr->pclass)];
    PlayerClass pc(player_ptr);
    if (pc.equals(PlayerClassType::PRIEST)) {
        if (is_good_realm(player_ptr->realm1)) {
            choices &= ~(CH_DEATH | CH_DAEMON);
        } else {
            choices &= ~(CH_LIFE | CH_CRUSADE);
        }
    }

    const auto tval = o_ptr->bi_key.tval();
    if ((tval == ItemKindType::MUSIC_BOOK) && pc.equals(PlayerClassType::BARD)) {
        return true;
    }

    if (!is_magic(tval2realm(tval))) {
        return false;
    }

    return (get_realm1_book(player_ptr) == tval) || (get_realm2_book(player_ptr) == tval) || (choices & (0x0001U << (tval2realm(tval) - 1)));
}
