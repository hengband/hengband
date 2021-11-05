﻿/*!
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
#include "object-use/item-use-checker.h"
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
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 使うオブジェクトの所持品ID
 */
ObjectUseEntity::ObjectUseEntity(PlayerType* player_ptr, INVENTORY_IDX item)
    : player_ptr(player_ptr)
    , item(item)
{
}

/*!
 * @brief 杖を使う
 */
void ObjectUseEntity::execute()
{
    auto use_charge = true;
    auto *o_ptr = ref_item(this->player_ptr, this->item);
    if ((this->item < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
        return;
    }

    PlayerEnergy(this->player_ptr).set_player_turn_energy(100);
    if (!this->check_can_use()) {
        return;
    }

    auto lev = k_info[o_ptr->k_idx].level;
    if (lev > 50) {
        lev = 50 + (lev - 50) / 2;
    }

    auto chance = this->player_ptr->skill_dev;
    if (this->player_ptr->confused) {
        chance = chance / 2;
    }

    chance = chance - lev;
    if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1)) {
        chance = USE_DEVICE;
    }

    if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || (this->player_ptr->pclass == PlayerClassType::BERSERKER)) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("杖をうまく使えなかった。", "You failed to use the staff properly."));
        sound(SOUND_FAIL);
        return;
    }

    if (o_ptr->pval <= 0) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("この杖にはもう魔力が残っていない。", "The staff has no charges left."));
        o_ptr->ident |= IDENT_EMPTY;
        this->player_ptr->update |= PU_COMBINE | PU_REORDER;
        this->player_ptr->window_flags |= PW_INVEN;
        return;
    }

    sound(SOUND_ZAP);
    auto ident = staff_effect(this->player_ptr, o_ptr->sval, &use_charge, false, false, o_ptr->is_aware());
    if (!(o_ptr->is_aware())) {
        chg_virtue(this->player_ptr, V_PATIENCE, -1);
        chg_virtue(this->player_ptr, V_CHANCE, 1);
        chg_virtue(this->player_ptr, V_KNOWLEDGE, -1);
    }

    /*
     * Temporarily remove the flags for updating the inventory so
     * gain_exp() does not reorder the inventory before the charge
     * is deducted from the staff.
     */
    BIT_FLAGS inventory_flags = PU_COMBINE | PU_REORDER | (this->player_ptr->update & PU_AUTODESTROY);
    reset_bits(this->player_ptr->update, PU_COMBINE | PU_REORDER | PU_AUTODESTROY);
    object_tried(o_ptr);
    if (ident && !o_ptr->is_aware()) {
        object_aware(this->player_ptr, o_ptr);
        gain_exp(this->player_ptr, (lev + (this->player_ptr->lev >> 1)) / this->player_ptr->lev);
    }

    set_bits(this->player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_PLAYER | PW_FLOOR_ITEM_LIST);
    set_bits(this->player_ptr->update, inventory_flags);
    if (!use_charge) {
        return;
    }

    o_ptr->pval--;
    if ((this->item >= 0) && (o_ptr->number > 1)) {
        object_type forge;
        auto *q_ptr = &forge;
        q_ptr->copy_from(o_ptr);
        q_ptr->number = 1;
        o_ptr->pval++;
        o_ptr->number--;
        this->item = store_item_to_inventory(this->player_ptr, q_ptr);
        msg_print(_("杖をまとめなおした。", "You unstack your staff."));
    }

    if (this->item >= 0) {
        inven_item_charges(this->player_ptr, this->item);
    } else {
        floor_item_charges(this->player_ptr->current_floor_ptr, 0 - this->item);
    }
}

bool ObjectUseEntity::check_can_use()
{
    if (cmd_limit_time_walk(this->player_ptr)) {
        return false;
    }

    return ItemUseChecker(this->player_ptr).check_stun(_("朦朧としていて杖を振れなかった！", "You were not able to use it by the stun!"));
}
