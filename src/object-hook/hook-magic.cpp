#include "object-hook/hook-magic.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/tr-types.h"
#include "object/object-flags.h"
#include "perception/object-perception.h"
#include "player/player-class.h"
#include "player/player-realm.h"
#include "realm/realm-names-table.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"

/*!
 * @brief オブジェクトをプレイヤーが魔道具として発動できるかを判定する /
 * Hook to determine if an object is activatable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 魔道具として発動可能ならばTRUEを返す
 */
bool item_tester_hook_activate(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    TrFlags flags;
    if (!object_is_known(o_ptr))
        return false;

    object_flags(player_ptr, o_ptr, flags);
    if (has_flag(flags, TR_ACTIVATE))
        return true;

    return false;
}

/*!
 * @brief オブジェクトをプレイヤーが簡易使用コマンドで利用できるかを判定する /
 * Hook to determine if an object is useable
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return 利用可能ならばTRUEを返す
 */
bool item_tester_hook_use(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    TrFlags flags;
    if (o_ptr->tval == player_ptr->tval_ammo)
        return true;

    switch (o_ptr->tval) {
    case TV_SPIKE:
    case TV_STAFF:
    case TV_WAND:
    case TV_ROD:
    case TV_SCROLL:
    case TV_POTION:
    case TV_FOOD: {
        return true;
    }

    default: {
        int i;

        if (!object_is_known(o_ptr))
            return false;
        for (i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
            if (&player_ptr->inventory_list[i] == o_ptr) {
                object_flags(player_ptr, o_ptr, flags);
                if (has_flag(flags, TR_ACTIVATE))
                    return true;
            }
        }
    }
    }

    return false;
}

/*!
 * @brief 魔力充填が可能なアイテムかどうか判定する /
 * Hook for "get_item()".  Determine if something is rechargable.
 * @param o_ptr 判定するアイテムの情報参照ポインタ
 * @return 魔力充填が可能ならばTRUEを返す
 */
bool item_tester_hook_recharge(player_type *player_ptr, object_type *o_ptr)
{
    /* Unused */
    (void)player_ptr;

    return (o_ptr->tval == TV_STAFF) || (o_ptr->tval == TV_WAND) || (o_ptr->tval == TV_ROD);
}

/*!
 * @brief オブジェクトがプレイヤーが使用可能な魔道書かどうかを判定する
 * @param o_ptr 判定したいオブ会ジェクトの構造体参照ポインタ
 * @return 学習できる魔道書ならばTRUEを返す
 */
bool item_tester_learn_spell(player_type *player_ptr, object_type *o_ptr)
{
    int32_t choices = realm_choices2[player_ptr->pclass];
    if (player_ptr->pclass == CLASS_PRIEST) {
        if (is_good_realm(player_ptr->realm1)) {
            choices &= ~(CH_DEATH | CH_DAEMON);
        } else {
            choices &= ~(CH_LIFE | CH_CRUSADE);
        }
    }

    if ((o_ptr->tval < TV_LIFE_BOOK) || (o_ptr->tval > (TV_LIFE_BOOK + static_cast<int>(MAX_REALM) - 1)))
        return false;

    if ((o_ptr->tval == TV_MUSIC_BOOK) && (player_ptr->pclass == CLASS_BARD))
        return true;
    else if (!is_magic(tval2realm(o_ptr->tval)))
        return false;

    return (get_realm1_book(player_ptr) == o_ptr->tval) || (get_realm2_book(player_ptr) == o_ptr->tval) || (choices & (0x0001U << (tval2realm(o_ptr->tval) - 1)));
}

/*!
 * @brief オブジェクトが高位の魔法書かどうかを判定する
 * @param o_ptr 判定したいオブジェクトの構造体参照ポインタ
 * @return オブジェクトが高位の魔法書ならばTRUEを返す
 */
bool item_tester_high_level_book(object_type *o_ptr)
{
    if ((o_ptr->tval == TV_LIFE_BOOK) || (o_ptr->tval == TV_SORCERY_BOOK) || (o_ptr->tval == TV_NATURE_BOOK) || (o_ptr->tval == TV_CHAOS_BOOK)
        || (o_ptr->tval == TV_DEATH_BOOK) || (o_ptr->tval == TV_TRUMP_BOOK) || (o_ptr->tval == TV_CRAFT_BOOK) || (o_ptr->tval == TV_DEMON_BOOK)
        || (o_ptr->tval == TV_CRUSADE_BOOK) || (o_ptr->tval == TV_MUSIC_BOOK) || (o_ptr->tval == TV_HEX_BOOK)) {
        if (o_ptr->sval > 1)
            return true;
        else
            return false;
    }

    return false;
}
