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
// ItemTester item_tester_hook;

ItemTester::ItemTester(std::function<bool(const object_type *)> pred)
{
    set_tester(pred);
}

ItemTester::ItemTester(std::function<bool(player_type *, const object_type *)> pred)
    : tester(std::move(pred))
{
}

void ItemTester::set_tester(std::function<bool(const object_type *)> pred)
{
    tester = [pred = std::move(pred)](player_type *, const object_type *o_ptr) { return pred(o_ptr); };
}

void ItemTester::set_tester(std::function<bool(player_type *, const object_type *)> pred)
{
    tester = std::move(pred);
}

/*!
 * @brief アイテムが条件を満たしているか調べる
 * Check an item against the item tester info
 * @param o_ptr 判定を行いたいオブジェクト構造体参照ポインタ
 * @return アイテムが条件を満たしているならtrue、その他いくつかの例外に応じてtrue/falseを返す。
 */
bool ItemTester::okay(player_type *player_ptr, const object_type *o_ptr, tval_type tval) const
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

    if (!this->tester)
        return true;

    return this->tester(player_ptr, o_ptr);
}
