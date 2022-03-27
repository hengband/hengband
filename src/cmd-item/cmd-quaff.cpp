/*!
 * @brief プレイヤーの飲むコマンド実装
 * @date 2018/09/07
 * @author deskull
 */

#include "cmd-item/cmd-quaff.h"
#include "action/action-limited.h"
#include "floor/floor-object.h"
#include "object-hook/hook-expendable.h"
#include "object-use/quaff/quaff-execution.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "realm/realm-hex-numbers.h"
#include "spell-realm/spells-hex.h"
#include "status/action-setter.h"
#include "system/player-type-definition.h"

/*!
 * @brief 薬を飲むコマンドのメインルーチン /
 * Quaff some potion (from the pack or floor)
 */
void do_cmd_quaff_potion(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode) {
        return;
    }

    if (!SpellHex(player_ptr).is_spelling_specific(HEX_INHALE) && cmd_limit_arena(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU, SamuraiStanceType::KOUKIJIN });

    concptr q = _("どの薬を飲みますか? ", "Quaff which potion? ");
    concptr s = _("飲める薬がない。", "You have no potions to quaff.");

    OBJECT_IDX item;
    if (!choose_object(player_ptr, &item, q, s, (USE_INVEN | USE_FLOOR), FuncItemTester(item_tester_hook_quaff, player_ptr))) {
        return;
    }

    ObjectQuaffEntity(player_ptr).execute(item);
}
