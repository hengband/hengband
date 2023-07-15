#include "object/lite-processor.h"
#include "core/disturbance.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "inventory/inventory-slot-types.h"
#include "object-enchant/object-ego.h"
#include "object/tval-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "timed-effect/player-blindness.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 10ゲームターンが進行する毎に光源の寿命を減らす処理
 * / Handle burning fuel every 10 game turns
 */
void reduce_lite_life(PlayerType *player_ptr)
{
    auto *o_ptr = &player_ptr->inventory_list[INVEN_LITE];
    if (o_ptr->bi_key.tval() != ItemKindType::LITE) {
        return;
    }

    if (o_ptr->is_fixed_artifact() || (o_ptr->bi_key.sval() == SV_LITE_FEANOR) || (o_ptr->fuel <= 0)) {
        return;
    }

    if (o_ptr->ego_idx == EgoType::LITE_LONG) {
        if (w_ptr->game_turn % (TURNS_PER_TICK * 2)) {
            o_ptr->fuel--;
        }
    } else {
        o_ptr->fuel--;
    }

    notice_lite_change(player_ptr, o_ptr);
}

/*!
 * @brief 寿命つき光源の警告メッセージ処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param o_ptr 現在光源として使っているオブジェクトの構造体参照ポインタ
 */
void notice_lite_change(PlayerType *player_ptr, ItemEntity *o_ptr)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if ((o_ptr->fuel < 100) || (!(o_ptr->fuel % 100))) {
        rfu.set_flag(SubWindowRedrawingFlag::EQUIPMENT);
    }

    if (player_ptr->effects()->blindness()->is_blind()) {
        if (o_ptr->fuel == 0) {
            o_ptr->fuel++;
        }
    } else if (o_ptr->fuel == 0) {
        disturb(player_ptr, false, true);
        msg_print(_("明かりが消えてしまった！", "Your light has gone out!"));
        static constexpr auto flags = {
            StatusRecalculatingFlag::TORCH,
            StatusRecalculatingFlag::BONUS,
        };
        rfu.set_flags(flags);
    } else if (o_ptr->ego_idx == EgoType::LITE_LONG) {
        if ((o_ptr->fuel < 50) && (!(o_ptr->fuel % 5)) && (w_ptr->game_turn % (TURNS_PER_TICK * 2))) {
            if (disturb_minor) {
                disturb(player_ptr, false, true);
            }
            msg_print(_("明かりが微かになってきている。", "Your light is growing faint."));
        }
    } else if ((o_ptr->fuel < 100) && (!(o_ptr->fuel % 10))) {
        if (disturb_minor) {
            disturb(player_ptr, false, true);
        }
        msg_print(_("明かりが微かになってきている。", "Your light is growing faint."));
    }
}
