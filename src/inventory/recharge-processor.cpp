#include "inventory/recharge-processor.h"
#include "core/disturbance.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "hpmp/hp-mp-regenerator.h"
#include "inventory/inventory-slot-types.h"
#include "object/object-kind.h"
#include "system/floor-type-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/quarks.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief
 * !!を刻んだ魔道具の時間経過による再充填を知らせる処理 /
 * If player has inscribed the object with "!!", let him know when it's recharged. -LM-
 * @param o_ptr 対象オブジェクトの構造体参照ポインタ
 */
static void recharged_notice(player_type *player_ptr, object_type *o_ptr)
{
    if (!o_ptr->inscription)
        return;

    concptr s = angband_strchr(quark_str(o_ptr->inscription), '!');
    while (s) {
        if (s[1] == '!') {
            GAME_TEXT o_name[MAX_NLEN];
            describe_flavor(player_ptr, o_name, o_ptr, (OD_OMIT_PREFIX | OD_NAME_ONLY));
#ifdef JP
            msg_format("%sは再充填された。", o_name);
#else
            if (o_ptr->number > 1)
                msg_format("Your %s are recharged.", o_name);
            else
                msg_format("Your %s is recharged.", o_name);
#endif
            disturb(player_ptr, false, false);
            return;
        }

        s = angband_strchr(s + 1, '!');
    }
}

/*!
 * @brief 10ゲームターンが進行するごとに魔道具の自然充填を行う処理
 * / Handle recharging objects once every 10 game turns
 */
void recharge_magic_items(player_type *player_ptr)
{
    int i;
    bool changed;

    for (changed = false, i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        if (o_ptr->timeout > 0) {
            o_ptr->timeout--;
            if (!o_ptr->timeout) {
                recharged_notice(player_ptr, o_ptr);
                changed = true;
            }
        }
    }

    if (changed) {
        player_ptr->window_flags |= (PW_EQUIP);
        wild_regen = 20;
    }

    /*
     * Recharge rods.  Rods now use timeout to control charging status,
     * and each charging rod in a stack decreases the stack's timeout by
     * one per turn. -LM-
     */
    for (changed = false, i = 0; i < INVEN_PACK; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];
        object_kind *k_ptr = &k_info[o_ptr->k_idx];
        if (!o_ptr->k_idx)
            continue;

        if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout)) {
            TIME_EFFECT temp = (o_ptr->timeout + (k_ptr->pval - 1)) / k_ptr->pval;
            if (temp > o_ptr->number)
                temp = (TIME_EFFECT)o_ptr->number;

            o_ptr->timeout -= temp;
            if (o_ptr->timeout < 0)
                o_ptr->timeout = 0;

            if (!(o_ptr->timeout)) {
                recharged_notice(player_ptr, o_ptr);
                changed = true;
            } else if (o_ptr->timeout % k_ptr->pval) {
                changed = true;
            }
        }
    }

    if (changed) {
        player_ptr->window_flags |= (PW_INVEN);
        wild_regen = 20;
    }

    for (i = 1; i < player_ptr->current_floor_ptr->o_max; i++) {
        object_type *o_ptr = &player_ptr->current_floor_ptr->o_list[i];
        if (!o_ptr->is_valid())
            continue;

        if ((o_ptr->tval == TV_ROD) && (o_ptr->timeout)) {
            o_ptr->timeout -= (TIME_EFFECT)o_ptr->number;
            if (o_ptr->timeout < 0)
                o_ptr->timeout = 0;
        }
    }
}
