﻿/*!
 * @brief プレイヤーの飲むコマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "cmd-item/cmd-quaff.h"
#include "action/action-limited.h"
#include "floor/floor-object.h"
#include "object-hook/hook-expendable.h"
#include "object-use/quaff-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"

/*!
 * @brief 薬を飲むコマンドのメインルーチン /
 * Quaff some potion (from the pack or floor)
 * @return なし
 */
void do_cmd_quaff_potion(player_type *creature_ptr)
{
    if (creature_ptr->wild_mode)
        return;

    if (cmd_limit_arena(creature_ptr))
        return;

    if (creature_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
        set_action(creature_ptr, ACTION_NONE);

    item_tester_hook = item_tester_hook_quaff;
    concptr q = _("どの薬を飲みますか? ", "Quaff which potion? ");
    concptr s = _("飲める薬がない。", "You have no potions to quaff.");

    OBJECT_IDX item;
    if (!choose_object(creature_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), TV_NONE))
        return;

    exe_quaff_potion(creature_ptr, item);
}
