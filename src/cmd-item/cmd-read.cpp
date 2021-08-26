/*!
 * @todo いずれcmd-item.c/h に統合したい
 * @brief プレイヤーの読むコマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "cmd-item/cmd-read.h"
#include "action/action-limited.h"
#include "floor/floor-object.h"
#include "object-hook/hook-expendable.h"
#include "object-use/read-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "perception/object-perception.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/player-type-definition.h"

/*!
 * @brief 読むコマンドのメインルーチン /
 * Eat some food (from the pack or floor)
 */
void do_cmd_read_scroll(player_type *creature_ptr)
{
    if (creature_ptr->wild_mode || cmd_limit_arena(creature_ptr))
        return;

    if (creature_ptr->special_defense & (KATA_MUSOU | KATA_KOUKIJIN))
        set_action(creature_ptr, ACTION_NONE);

    if (cmd_limit_blind(creature_ptr) || cmd_limit_confused(creature_ptr))
        return;

    item_tester_hook = make_item_tester(object_is_readable);
    concptr q = _("どの巻物を読みますか? ", "Read which scroll? ");
    concptr s = _("読める巻物がない。", "You have no scrolls to read.");
    object_type *o_ptr;
    OBJECT_IDX item;
    o_ptr = choose_object(creature_ptr, &item, q, s, USE_INVEN | USE_FLOOR, TV_NONE);
    if (!o_ptr)
        return;

    exe_read(creature_ptr, item, object_is_aware(o_ptr));
}
