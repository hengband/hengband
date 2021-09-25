/*!
 * @brief 杖を振る処理
 * @date 2021/09/25
 * @author Hourier
 */
#include "object-use/use-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-usestaff.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-status/player-energy.h"
#include "status/experience.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

/*!
 * @brief 杖を使うコマンドのサブルーチン /
 * Use a staff.			-RAK-
 * @param item 使うオブジェクトの所持品ID
 * @details
 * One charge of one staff disappears.
 * Hack -- staffs of identify can be "cancelled".
 */
void exe_use_staff(player_type *player_ptr, INVENTORY_IDX item)
{
    int ident, chance, lev;
    object_type *o_ptr;

    /* Hack -- let staffs of identify get aborted */
    bool use_charge = true;

    o_ptr = ref_item(player_ptr, item);

    /* Mega-Hack -- refuse to use a pile from the ground */
    if ((item < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    lev = k_info[o_ptr->k_idx].level;
    if (lev > 50)
        lev = 50 + (lev - 50) / 2;

    /* Base chance of success */
    chance = player_ptr->skill_dev;

    /* Confusion hurts skill */
    if (player_ptr->confused)
        chance = chance / 2;

    /* Hight level objects are harder */
    chance = chance - lev;

    /* Give everyone a (slight) chance */
    if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1)) {
        chance = USE_DEVICE;
    }

    if (cmd_limit_time_walk(player_ptr))
        return;

    /* Roll for usage */
    if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || (player_ptr->pclass == CLASS_BERSERKER)) {
        if (flush_failure)
            flush();
        msg_print(_("杖をうまく使えなかった。", "You failed to use the staff properly."));
        sound(SOUND_FAIL);
        return;
    }

    /* Notice empty staffs */
    if (o_ptr->pval <= 0) {
        if (flush_failure)
            flush();
        msg_print(_("この杖にはもう魔力が残っていない。", "The staff has no charges left."));
        o_ptr->ident |= IDENT_EMPTY;
        player_ptr->update |= PU_COMBINE | PU_REORDER;
        player_ptr->window_flags |= PW_INVEN;

        return;
    }

    sound(SOUND_ZAP);

    ident = staff_effect(player_ptr, o_ptr->sval, &use_charge, false, false, o_ptr->is_aware());

    if (!(o_ptr->is_aware())) {
        chg_virtue(player_ptr, V_PATIENCE, -1);
        chg_virtue(player_ptr, V_CHANCE, 1);
        chg_virtue(player_ptr, V_KNOWLEDGE, -1);
    }

    /*
     * Temporarily remove the flags for updating the inventory so
     * gain_exp() does not reorder the inventory before the charge
     * is deducted from the staff.
     */
    BIT_FLAGS inventory_flags = (PU_COMBINE | PU_REORDER | (player_ptr->update & PU_AUTODESTROY));
    reset_bits(player_ptr->update, PU_COMBINE | PU_REORDER | PU_AUTODESTROY);

    /* Tried the item */
    object_tried(o_ptr);

    /* An identification was made */
    if (ident && !o_ptr->is_aware()) {
        object_aware(player_ptr, o_ptr);
        gain_exp(player_ptr, (lev + (player_ptr->lev >> 1)) / player_ptr->lev);
    }

    set_bits(player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_PLAYER | PW_FLOOR_ITEM_LIST);
    set_bits(player_ptr->update, inventory_flags);

    /* Hack -- some uses are "free" */
    if (!use_charge)
        return;

    /* Use a single charge */
    o_ptr->pval--;

    /* XXX Hack -- unstack if necessary */
    if ((item >= 0) && (o_ptr->number > 1)) {
        object_type forge;
        object_type *q_ptr;
        q_ptr = &forge;
        q_ptr->copy_from(o_ptr);

        /* Modify quantity */
        q_ptr->number = 1;

        /* Restore the charges */
        o_ptr->pval++;

        /* Unstack the used item */
        o_ptr->number--;
        item = store_item_to_inventory(player_ptr, q_ptr);

        msg_print(_("杖をまとめなおした。", "You unstack your staff."));
    }

    /* Describe charges in the pack */
    if (item >= 0) {
        inven_item_charges(player_ptr, item);
    }

    /* Describe charges on the floor */
    else {
        floor_item_charges(player_ptr->current_floor_ptr, 0 - item);
    }
}
