#include "view/object-describer.h"
#include "object/object-flavor.h"
#include "object/object-hook.h"
#include "object/special-object-flags.h"

/*!
 * @brief 魔道具の使用回数の残量を示すメッセージを表示する /
 * Describe the charges on an item in the inventory.
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item 残量を表示したいプレイヤーのアイテム所持スロット
 * @return なし
 */
void inven_item_charges(player_type *owner_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr = &owner_ptr->inventory_list[item];
    if ((o_ptr->tval != TV_STAFF) && (o_ptr->tval != TV_WAND))
        return;
    if (!object_is_known(o_ptr))
        return;

#ifdef JP
    if (o_ptr->pval <= 0) {
        msg_print("もう魔力が残っていない。");
    } else {
        msg_format("あと %d 回分の魔力が残っている。", o_ptr->pval);
    }
#else
    if (o_ptr->pval != 1) {
        msg_format("You have %d charges remaining.", o_ptr->pval);
    }

    else {
        msg_format("You have %d charge remaining.", o_ptr->pval);
    }
#endif
}

/*!
 * @brief アイテムの残り所持数メッセージを表示する /
 * Describe an item in the inventory.
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param item 残量を表示したいプレイヤーのアイテム所持スロット
 * @return なし
 */
void inven_item_describe(player_type *owner_ptr, INVENTORY_IDX item)
{
    object_type *o_ptr = &owner_ptr->inventory_list[item];
    GAME_TEXT o_name[MAX_NLEN];
    object_desc(owner_ptr, o_name, o_ptr, 0);
#ifdef JP
    if (o_ptr->number <= 0) {
        msg_format("もう%sを持っていない。", o_name);
    } else {
        msg_format("まだ %sを持っている。", o_name);
    }
#else
    msg_format("You have %s.", o_name);
#endif
}
