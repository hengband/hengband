﻿#include "spell-realm/spells-sorcery.h"
#include "core/asking-player.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "floor/floor-object.h"
#include "game-option/input-options.h"
#include "inventory/inventory-object.h"
#include "io/input-key-requester.h"
#include "object-hook/hook-expendable.h"
#include "object/item-use-flags.h"
#include "object/object-value.h"
#include "system/object-type-definition.h"
#include "view/display-messages.h"

/*!
 * @brief アイテムの価値に応じた錬金術処理 /
 * Turns an object into gold, gain some of its value in a shop
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return 処理が実際に行われたらTRUEを返す
 */
bool alchemy(player_type *caster_ptr)
{
    bool force = FALSE;
    if (command_arg > 0)
        force = TRUE;

    concptr q = _("どのアイテムを金に変えますか？", "Turn which item to gold? ");
    concptr s = _("金に変えられる物がありません。", "You have nothing to turn to gold.");
    OBJECT_IDX item;
    object_type *o_ptr;
    o_ptr = choose_object(caster_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), TV_NONE);
    if (!o_ptr)
        return FALSE;

    int amt = 1;
    if (o_ptr->number > 1) {
        amt = get_quantity(NULL, o_ptr->number);
        if (amt <= 0)
            return FALSE;
    }

    ITEM_NUMBER old_number = o_ptr->number;
    o_ptr->number = amt;
    GAME_TEXT o_name[MAX_NLEN];
    describe_flavor(caster_ptr, o_name, o_ptr, 0);
    o_ptr->number = old_number;

    if (!force) {
        if (confirm_destroy || (object_value(caster_ptr, o_ptr) > 0)) {
            char out_val[MAX_NLEN + 40];
            sprintf(out_val, _("本当に%sを金に変えますか？", "Really turn %s to gold? "), o_name);
            if (!get_check(out_val))
                return FALSE;
        }
    }

    if (!can_player_destroy_object(caster_ptr, o_ptr)) {
        msg_format(_("%sを金に変えることに失敗した。", "You fail to turn %s to gold!"), o_name);
        return FALSE;
    }

    PRICE price = object_value_real(caster_ptr, o_ptr);
    if (price <= 0) {
        msg_format(_("%sをニセの金に変えた。", "You turn %s to fool's gold."), o_name);
        vary_item(caster_ptr, item, -amt);
        return TRUE;
    }

    price /= 3;

    if (amt > 1)
        price *= amt;

    if (price > 30000)
        price = 30000;
    msg_format(_("%sを＄%d の金に変えた。", "You turn %s to %ld coins worth of gold."), o_name, price);

    caster_ptr->au += price;
    caster_ptr->redraw |= PR_GOLD;
    caster_ptr->window_flags |= PW_PLAYER;
    vary_item(caster_ptr, item, -amt);
    return TRUE;
}
