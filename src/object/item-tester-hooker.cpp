/*!
 * @brief オブジェクトに関する汎用判定処理
 * @date 2018/09/24
 * @author deskull
 */

#include "object/item-tester-hooker.h"
#include "object/object-info.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-describer.h"

/*
 * Used during calls to "get_item()" and "show_inven()" and "show_equip()", and the choice window routines.
 */
ItemTester item_tester_hook;

/*!
 * @brief アイテムがitem_tester_hookグローバル関数ポインタの条件を満たしているかを返す汎用関数
 * Check an item against the item tester info
 * @param o_ptr 判定を行いたいオブジェクト構造体参照ポインタ
 * @return item_tester_hookの参照先が特にないならTRUE、その他いくつかの例外に応じてTRUE/FALSEを返す。
 */
bool item_tester_okay(player_type *player_ptr, const object_type *o_ptr, tval_type tval)
{
    if (!o_ptr->k_idx)
        return false;

    if (o_ptr->tval == TV_GOLD) {
        if (!show_gold_on_floor)
            return false;
    }

    if (tval) {
        if ((tval <= TV_DEATH_BOOK) && (tval >= TV_LIFE_BOOK))
            return check_book_realm(player_ptr, o_ptr->tval, o_ptr->sval);
        else if (tval != o_ptr->tval)
            return false;
    }

    if (item_tester_hook == NULL)
        return true;

    return item_tester_hook(player_ptr, o_ptr);
}

/*!
 * @brief オブジェクト判定関数を item_tester_hook に渡すためのラッパー関数を生成する
 *
 * @param pred オブジェクトへのポインタを受け取り、オブジェクトが条件に適合するかどうかを返す関数
 * @return item_tester_hook に渡すラッパー関数を返す
 */
ItemTester make_item_tester(std::function<bool(const object_type *)> pred)
{
    return [pred = std::move(pred)](player_type *, const object_type *o_ptr) { return pred(o_ptr); };
}
