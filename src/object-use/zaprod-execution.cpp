/*!
 * @brief ロッドを振る処理
 * @date 2021/09/25
 * @author Hourier
 */
#include "object-use/zaprod-execution.h"
#include "action/action-limited.h"
#include "avatar/avatar.h"
#include "cmd-item/cmd-zaprod.h" // 相互依存。暫定的措置、後で何とかする.
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player-status/player-energy.h"
#include "status/experience.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-rod-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief コンストラクタ
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param item 使うオブジェクトの所持品ID
 */
ObjectZapRodEntity::ObjectZapRodEntity(player_type* player_ptr)
    : player_ptr(player_ptr)
{
}

/*!
 * @brief ロッドを使う
 */
void ObjectZapRodEntity::execute(INVENTORY_IDX item)
{
    int ident, chance, lev, fail;
    DIRECTION dir = 0;
    object_type *o_ptr;
    bool success;

    /* Hack -- let perception get aborted */
    bool use_charge = true;

    object_kind *k_ptr;

    o_ptr = ref_item(this->player_ptr, item);

    /* Mega-Hack -- refuse to zap a pile from the ground */
    if ((item < 0) && (o_ptr->number > 1)) {
        msg_print(_("まずはロッドを拾わなければ。", "You must first pick up the rods."));
        return;
    }

    /* Get a direction (unless KNOWN not to need it) */
    if (((o_ptr->sval >= SV_ROD_MIN_DIRECTION) && (o_ptr->sval != SV_ROD_HAVOC) && (o_ptr->sval != SV_ROD_AGGRAVATE) && (o_ptr->sval != SV_ROD_PESTICIDE))
        || !o_ptr->is_aware()) {
        /* Get a direction, allow cancel */
        if (!get_aim_dir(this->player_ptr, &dir))
            return;
    }

    PlayerEnergy(this->player_ptr).set_player_turn_energy(100);

    lev = k_info[o_ptr->k_idx].level;

    /* Base chance of success */
    chance = this->player_ptr->skill_dev;

    /* Confusion hurts skill */
    if (this->player_ptr->confused)
        chance = chance / 2;

    fail = lev + 5;
    if (chance > fail)
        fail -= (chance - fail) * 2;
    else
        chance -= (fail - chance) * 2;
    if (fail < USE_DEVICE)
        fail = USE_DEVICE;
    if (chance < USE_DEVICE)
        chance = USE_DEVICE;

    if (cmd_limit_time_walk(this->player_ptr))
        return;

    if (this->player_ptr->pclass == CLASS_BERSERKER)
        success = false;
    else if (chance > fail) {
        if (randint0(chance * 2) < fail)
            success = false;
        else
            success = true;
    } else {
        if (randint0(fail * 2) < chance)
            success = true;
        else
            success = false;
    }

    /* Roll for usage */
    if (!success) {
        if (flush_failure)
            flush();
        msg_print(_("うまくロッドを使えなかった。", "You failed to use the rod properly."));
        sound(SOUND_FAIL);
        return;
    }

    k_ptr = &k_info[o_ptr->k_idx];

    /* A single rod is still charging */
    if ((o_ptr->number == 1) && (o_ptr->timeout)) {
        if (flush_failure)
            flush();
        msg_print(_("このロッドはまだ魔力を充填している最中だ。", "The rod is still charging."));
        return;
    }
    /* A stack of rods lacks enough energy. */
    else if ((o_ptr->number > 1) && (o_ptr->timeout > k_ptr->pval * (o_ptr->number - 1))) {
        if (flush_failure)
            flush();
        msg_print(_("そのロッドはまだ充填中です。", "The rods are all still charging."));
        return;
    }

    sound(SOUND_ZAP);

    ident = rod_effect(this->player_ptr, o_ptr->sval, dir, &use_charge, false, false);

    /* Increase the timeout by the rod kind's pval. -LM- */
    if (use_charge)
        o_ptr->timeout += k_ptr->pval;
    this->player_ptr->update |= PU_COMBINE | PU_REORDER;

    if (!(o_ptr->is_aware())) {
        chg_virtue(this->player_ptr, V_PATIENCE, -1);
        chg_virtue(this->player_ptr, V_CHANCE, 1);
        chg_virtue(this->player_ptr, V_KNOWLEDGE, -1);
    }

    /* Tried the object */
    object_tried(o_ptr);

    /* Successfully determined the object function */
    if (ident && !o_ptr->is_aware()) {
        object_aware(this->player_ptr, o_ptr);
        gain_exp(this->player_ptr, (lev + (this->player_ptr->lev >> 1)) / this->player_ptr->lev);
    }

    set_bits(this->player_ptr->window_flags, PW_INVEN | PW_EQUIP | PW_PLAYER | PW_FLOOR_ITEM_LIST);
}
