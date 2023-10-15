/*!
 * @brief 杖を振る処理
 * @date 2021/09/25
 * @author Hourier
 */
#include "object-use/use-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-usestaff.h"
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-object.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/special-object-flags.h"
#include "object-use/item-use-checker.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "status/experience.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param i_idx 使うオブジェクトの所持品ID
 */
ObjectUseEntity::ObjectUseEntity(PlayerType *player_ptr, INVENTORY_IDX i_idx)
    : player_ptr(player_ptr)
    , i_idx(i_idx)
{
}

/*!
 * @brief 杖を使う
 */
void ObjectUseEntity::execute()
{
    auto use_charge = true;
    auto *o_ptr = ref_item(this->player_ptr, this->i_idx);
    if ((this->i_idx < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずは杖を拾わなければ。", "You must first pick up the staffs."));
        return;
    }

    PlayerEnergy(this->player_ptr).set_player_turn_energy(100);
    if (!this->check_can_use()) {
        return;
    }

    auto lev = o_ptr->get_baseitem().level;
    if (lev > 50) {
        lev = 50 + (lev - 50) / 2;
    }

    auto chance = this->player_ptr->skill_dev;
    if (this->player_ptr->effects()->confusion()->is_confused()) {
        chance = chance / 2;
    }

    chance = chance - lev;
    if ((chance < USE_DEVICE) && one_in_(USE_DEVICE - chance + 1)) {
        chance = USE_DEVICE;
    }

    if ((chance < USE_DEVICE) || (randint1(chance) < USE_DEVICE) || PlayerClass(this->player_ptr).equals(PlayerClassType::BERSERKER)) {
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
        auto &rfu = RedrawingFlagsUpdater::get_instance();
        static constexpr auto flags = {
            StatusRecalculatingFlag::COMBINATION,
            StatusRecalculatingFlag::REORDER,
        };
        rfu.set_flags(flags);
        rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        return;
    }

    sound(SOUND_ZAP);
    auto ident = staff_effect(this->player_ptr, o_ptr->bi_key.sval().value(), &use_charge, false, false, o_ptr->is_aware());
    if (!(o_ptr->is_aware())) {
        chg_virtue(this->player_ptr, Virtue::PATIENCE, -1);
        chg_virtue(this->player_ptr, Virtue::CHANCE, 1);
        chg_virtue(this->player_ptr, Virtue::KNOWLEDGE, -1);
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    using Srf = StatusRecalculatingFlag;
    EnumClassFlagGroup<Srf> flags_srf = { Srf::COMBINATION, Srf::REORDER };
    if (rfu.has(Srf::AUTO_DESTRUCTION)) {
        flags_srf.set(Srf::AUTO_DESTRUCTION);
    }

    rfu.reset_flags(flags_srf);
    object_tried(o_ptr);
    if (ident && !o_ptr->is_aware()) {
        object_aware(this->player_ptr, o_ptr);
        gain_exp(this->player_ptr, (lev + (this->player_ptr->lev >> 1)) / this->player_ptr->lev);
    }

    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
    rfu.set_flags(flags_srf);
    if (!use_charge) {
        return;
    }

    o_ptr->pval--;
    if ((this->i_idx >= 0) && (o_ptr->number > 1)) {
        ItemEntity forge;
        auto *q_ptr = &forge;
        q_ptr->copy_from(o_ptr);
        q_ptr->number = 1;
        o_ptr->pval++;
        o_ptr->number--;
        this->i_idx = store_item_to_inventory(this->player_ptr, q_ptr);
        msg_print(_("杖をまとめなおした。", "You unstack your staff."));
    }

    if (this->i_idx >= 0) {
        inven_item_charges(this->player_ptr->inventory_list[this->i_idx]);
    } else {
        floor_item_charges(this->player_ptr->current_floor_ptr, 0 - this->i_idx);
    }
}

bool ObjectUseEntity::check_can_use()
{
    if (cmd_limit_time_walk(this->player_ptr)) {
        return false;
    }

    return ItemUseChecker(this->player_ptr).check_stun(_("朦朧としていて杖を振れなかった！", "You are too stunned to use it!"));
}
