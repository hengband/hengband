/*!
 * @brief 自動破壊の実行
 * @date 2020/04/25
 * @author Hourier
 */

#include "autopick/autopick-destroyer.h"
#include "autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "core/disturbance.h"
#include "core/player-update-types.h"
#include "flavor/flavor-describer.h"
#include "game-option/auto-destruction-options.h"
#include "game-option/input-options.h"
#include "monster-race/monster-race.h"
#include "object-enchant/object-ego.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-quest.h"
#include "object-hook/hook-weapon.h"
#include "object/object-mark-types.h"
#include "object/object-value.h"
#include "perception/object-perception.h"
#include "player/player-race-types.h"
#include "sv-definition/sv-other-types.h"
#include "sv-definition/sv-wand-types.h"
#include "util/string-processor.h"
#include "view/display-messages.h"

/*!
 * @brief クラス依存のアイテム破壊を調べる
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param o_ptr アイテムへの参照ポインタ
 * @return 特別なクラス、かつそのクラス特有のアイテムであればFALSE、それ以外はTRUE
 */
static bool is_leave_special_item(player_type *player_ptr, object_type *o_ptr)
{
    if (!leave_special)
        return TRUE;

    if (player_ptr->prace == RACE_BALROG) {
        if (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_CORPSE && angband_strchr("pht", r_info[o_ptr->pval].d_char))
            return FALSE;
    } else if (player_ptr->pclass == CLASS_ARCHER) {
        if (o_ptr->tval == TV_SKELETON || (o_ptr->tval == TV_CORPSE && o_ptr->sval == SV_SKELETON))
            return FALSE;
    } else if (player_ptr->pclass == CLASS_NINJA) {
        if (o_ptr->tval == TV_LITE && o_ptr->name2 == EGO_LITE_DARKNESS && object_is_known(o_ptr))
            return FALSE;
    } else if (player_ptr->pclass == CLASS_BEASTMASTER || player_ptr->pclass == CLASS_CAVALRY) {
        if (o_ptr->tval == TV_WAND && o_ptr->sval == SV_WAND_HEAL_MONSTER && object_is_aware(o_ptr))
            return FALSE;
    }

    return TRUE;
}

/*
 * Automatically destroy items in this grid.
 */
static bool is_opt_confirm_destroy(player_type *player_ptr, object_type *o_ptr)
{
    if (!destroy_items)
        return FALSE;

    if (leave_worth)
        if (object_value(player_ptr, o_ptr) > 0)
            return FALSE;

    if (leave_equip)
        if (object_is_weapon_armour_ammo(player_ptr, o_ptr))
            return FALSE;

    if (leave_chest)
        if ((o_ptr->tval == TV_CHEST) && o_ptr->pval)
            return FALSE;

    if (leave_wanted)
        if (object_is_bounty(player_ptr, o_ptr))
            return FALSE;

    if (leave_corpse)
        if (o_ptr->tval == TV_CORPSE)
            return FALSE;

    if (leave_junk)
        if ((o_ptr->tval == TV_SKELETON) || (o_ptr->tval == TV_BOTTLE) || (o_ptr->tval == TV_JUNK) || (o_ptr->tval == TV_STATUE))
            return FALSE;

    if (!is_leave_special_item(player_ptr, o_ptr))
        return FALSE;

    if (o_ptr->tval == TV_GOLD)
        return FALSE;

    return TRUE;
}

void auto_destroy_item(player_type *player_ptr, object_type *o_ptr, int autopick_idx)
{
    bool destroy = FALSE;
    if (is_opt_confirm_destroy(player_ptr, o_ptr))
        destroy = TRUE;

    if (autopick_idx >= 0 && !(autopick_list[autopick_idx].action & DO_AUTODESTROY))
        destroy = FALSE;

    if (!always_pickup) {
        if (autopick_idx >= 0 && (autopick_list[autopick_idx].action & DO_AUTODESTROY))
            destroy = TRUE;
    }

    if (!destroy)
        return;

    disturb(player_ptr, FALSE, FALSE);
    if (!can_player_destroy_object(player_ptr, o_ptr)) {
        GAME_TEXT o_name[MAX_NLEN];
        describe_flavor(player_ptr, o_name, o_ptr, 0);
        msg_format(_("%sは破壊不能だ。", "You cannot auto-destroy %s."), o_name);
        return;
    }

    (void)COPY(&autopick_last_destroyed_object, o_ptr, object_type);
    o_ptr->marked |= OM_AUTODESTROY;
    player_ptr->update |= PU_AUTODESTROY;
}
