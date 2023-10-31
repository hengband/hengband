/*!
 * @brief ロッドを振る処理
 * @date 2021/09/25
 * @author Hourier
 */
#include "object-use/zaprod-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-zaprod.h" // 相互依存。暫定的措置、後で何とかする.
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-use/item-use-checker.h"
#include "object/object-info.h"
#include "perception/object-perception.h"
#include "player-base/player-class.h"
#include "player-status/player-energy.h"
#include "status/experience.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-rod-types.h"
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

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 使うオブジェクトの所持品ID
 */
ObjectZapRodEntity::ObjectZapRodEntity(PlayerType *player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief ロッドを使う
 */
void ObjectZapRodEntity::execute(INVENTORY_IDX i_idx)
{
    DIRECTION dir = 0;
    auto use_charge = true;
    auto *o_ptr = ref_item(this->player_ptr, i_idx);
    if ((i_idx < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずはロッドを拾わなければ。", "You must first pick up the rods."));
        return;
    }

    if (o_ptr->is_aiming_rod() || !o_ptr->is_aware()) {
        if (!get_aim_dir(this->player_ptr, &dir)) {
            return;
        }
    }

    PlayerEnergy(this->player_ptr).set_player_turn_energy(100);
    if (!this->check_can_zap()) {
        return;
    }

    auto lev = o_ptr->get_baseitem().level;
    auto chance = this->player_ptr->skill_dev;
    if (this->player_ptr->effects()->confusion()->is_confused()) {
        chance = chance / 2;
    }

    auto fail = lev + 5;
    if (chance > fail) {
        fail -= (chance - fail) * 2;
    } else {
        chance -= (fail - chance) * 2;
    }

    if (fail < USE_DEVICE) {
        fail = USE_DEVICE;
    }

    if (chance < USE_DEVICE) {
        chance = USE_DEVICE;
    }

    bool success;
    if (PlayerClass(this->player_ptr).equals(PlayerClassType::BERSERKER)) {
        success = false;
    } else if (chance > fail) {
        success = randint0(chance * 2) >= fail;
    } else {
        success = randint0(fail * 2) < chance;
    }

    if (!success) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("うまくロッドを使えなかった。", "You failed to use the rod properly."));
        sound(SOUND_FAIL);
        return;
    }

    const auto &baseitem = o_ptr->get_baseitem();
    if ((o_ptr->number == 1) && (o_ptr->timeout)) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("このロッドはまだ魔力を充填している最中だ。", "The rod is still charging."));
        return;
    } else if ((o_ptr->number > 1) && (o_ptr->timeout > baseitem.pval * (o_ptr->number - 1))) {
        if (flush_failure) {
            flush();
        }

        msg_print(_("そのロッドはまだ充填中です。", "The rods are all still charging."));
        return;
    }

    sound(SOUND_ZAP);
    auto ident = rod_effect(this->player_ptr, o_ptr->bi_key.sval().value(), dir, &use_charge, false);
    if (use_charge) {
        o_ptr->timeout += baseitem.pval;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
    };
    rfu.set_flags(flags_srf);
    if (!(o_ptr->is_aware())) {
        chg_virtue(this->player_ptr, Virtue::PATIENCE, -1);
        chg_virtue(this->player_ptr, Virtue::CHANCE, 1);
        chg_virtue(this->player_ptr, Virtue::KNOWLEDGE, -1);
    }

    object_tried(o_ptr);
    if ((ident != 0) && !o_ptr->is_aware()) {
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
}

bool ObjectZapRodEntity::check_can_zap()
{
    if (cmd_limit_time_walk(this->player_ptr)) {
        return false;
    }

    return ItemUseChecker(this->player_ptr).check_stun(_("朦朧としていてロッドを振れなかった！", "You are too stunned to zap it!"));
}
