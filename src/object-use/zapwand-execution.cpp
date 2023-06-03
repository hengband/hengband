#include "object-use/zapwand-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-zapwand.h" // 相互依存。暫定的措置、後で何とかする.
#include "core/window-redrawer.h"
#include "floor/floor-object.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/special-object-flags.h"
#include "object-use/item-use-checker.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "status/experience.h"
#include "sv-definition/sv-wand-types.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "view/object-describer.h"

ObjectZapWandEntity::ObjectZapWandEntity(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief 魔法棒を使うコマンドのサブルーチン /
 * @param item 使うオブジェクトの所持品ID
 */
void ObjectZapWandEntity::execute(INVENTORY_IDX item)
{
    auto old_target_pet = target_pet;
    auto *o_ptr = ref_item(this->player_ptr, item);
    if ((item < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずは魔法棒を拾わなければ。", "You must first pick up the wands."));
        return;
    }

    const auto sval = o_ptr->bi_key.sval();
    if (o_ptr->is_aware() && (sval == SV_WAND_HEAL_MONSTER || sval == SV_WAND_HASTE_MONSTER)) {
        target_pet = true;
    }

    DIRECTION dir;
    if (!get_aim_dir(this->player_ptr, &dir)) {
        target_pet = old_target_pet;
        return;
    }

    target_pet = old_target_pet;
    PlayerEnergy(this->player_ptr).set_player_turn_energy(100);
    if (!this->check_can_zap()) {
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

        msg_print(_("魔法棒をうまく使えなかった。", "You failed to use the wand properly."));
        sound(SOUND_FAIL);
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (o_ptr->pval <= 0) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("この魔法棒にはもう魔力が残っていない。", "The wand has no charges left."));
        o_ptr->ident |= IDENT_EMPTY;
        static constexpr auto flags = {
            StatusRecalculatingFlag::COMBINATION,
            StatusRecalculatingFlag::REORDER,
        };
        rfu.set_flags(flags);
        rfu.set_flag(SubWindowRedrawingFlag::INVENTORY);
        return;
    }

    sound(SOUND_ZAP);
    auto ident = wand_effect(this->player_ptr, sval.value(), dir, false, false);
    using Srf = StatusRecalculatingFlag;
    EnumClassFlagGroup<Srf> flags_srf = { Srf::COMBINATION, Srf::REORDER };
    if (rfu.has(Srf::AUTO_DESTRUCTION)) {
        flags_srf.set(Srf::AUTO_DESTRUCTION);
    }

    rfu.reset_flags(flags_srf);
    if (!(o_ptr->is_aware())) {
        chg_virtue(this->player_ptr, Virtue::PATIENCE, -1);
        chg_virtue(this->player_ptr, Virtue::CHANCE, 1);
        chg_virtue(this->player_ptr, Virtue::KNOWLEDGE, -1);
    }

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
    o_ptr->pval--;
    if (item >= 0) {
        inven_item_charges(this->player_ptr->inventory_list[item]);
        return;
    }

    floor_item_charges(this->player_ptr->current_floor_ptr, 0 - item);
}

bool ObjectZapWandEntity::check_can_zap() const
{
    if (cmd_limit_time_walk(this->player_ptr)) {
        return false;
    }

    return ItemUseChecker(this->player_ptr).check_stun(_("朦朧としていて魔法棒を振れなかった！", "You are too stunned to zap it!"));
}
