﻿#include "view/object-describer.h"
#include "cmd-action/cmd-spell.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-generator.h"
#include "perception/object-perception.h"
#include "realm/realm-names-table.h"
#include "spell/spell-info.h"
#include "system/object-type-definition.h"
#include "term/term-color-types.h"
#include "view/display-messages.h"

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
    describe_flavor(owner_ptr, o_name, o_ptr, 0);
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

/*!
 * @brief 現在アクティブになっているウィンドウにオブジェクトの詳細を表示する /
 * Hack -- display an object kind in the current window
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param k_idx ベースアイテムの参照ID
 * @return なし
 * @details
 * Include list of usable spells for readible books
 */
void display_koff(player_type *owner_ptr, KIND_OBJECT_IDX k_idx)
{
    object_type forge;
    object_type *q_ptr;
    int sval;
    REALM_IDX use_realm;
    GAME_TEXT o_name[MAX_NLEN];
    for (int y = 0; y < Term->hgt; y++) {
        term_erase(0, y, 255);
    }

    if (!k_idx)
        return;
    q_ptr = &forge;

    object_prep(owner_ptr, q_ptr, k_idx);
    describe_flavor(owner_ptr, o_name, q_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY | OD_STORE));

    term_putstr(0, 0, -1, TERM_WHITE, o_name);
    sval = q_ptr->sval;
    use_realm = tval2realm(q_ptr->tval);

    if (owner_ptr->realm1 || owner_ptr->realm2) {
        if ((use_realm != owner_ptr->realm1) && (use_realm != owner_ptr->realm2))
            return;
    } else {
        if ((owner_ptr->pclass != CLASS_SORCERER) && (owner_ptr->pclass != CLASS_RED_MAGE))
            return;
        if (!is_magic(use_realm))
            return;
        if ((owner_ptr->pclass == CLASS_RED_MAGE) && (use_realm != REALM_ARCANE) && (sval > 1))
            return;
    }

    int num = 0;
    SPELL_IDX spells[64];

    for (int spell = 0; spell < 32; spell++) {
        if (fake_spell_flags[sval] & (1UL << spell)) {
            spells[num++] = spell;
        }
    }

    print_spells(owner_ptr, 0, spells, num, 2, 0, use_realm);
}
