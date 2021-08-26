#include "cmd-action/cmd-shoot.h"
#include "combat/shoot.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "mind/snipe-types.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-teleport.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "sv-definition/sv-bow-types.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief 射撃処理のメインルーチン
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param snipe_type ？？？
 * @todo Doxygenの加筆求む
 */
void do_cmd_fire(player_type *creature_ptr, SPELL_IDX snipe_type)
{
    OBJECT_IDX item;
    object_type *j_ptr, *ammo_ptr;
    if (creature_ptr->wild_mode)
        return;

    creature_ptr->is_fired = false;
    j_ptr = &creature_ptr->inventory_list[INVEN_BOW];
    if (!j_ptr->tval) {
        msg_print(_("射撃用の武器を持っていない。", "You have nothing to fire with."));
        flush();
        return;
    }

    if (j_ptr->sval == SV_CRIMSON) {
        msg_print(_("この武器は発動して使うもののようだ。", "It's already activated."));
        flush();
        return;
    }

    if (j_ptr->sval == SV_HARP) {
        msg_print(_("この武器で射撃はできない。", "It's not for firing."));
        flush();
        return;
    }

    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    concptr q = _("どれを撃ちますか? ", "Fire which item? ");
    concptr s = _("発射されるアイテムがありません。", "You have nothing to fire.");
    ammo_ptr = choose_object(creature_ptr, &item, q, s, USE_INVEN | USE_FLOOR, TvalItemTester(creature_ptr->tval_ammo));
    if (!ammo_ptr) {
        flush();
        return;
    }

    exe_fire(creature_ptr, item, j_ptr, snipe_type);
    if (!creature_ptr->is_fired || creature_ptr->pclass != CLASS_SNIPER)
        return;

    if (snipe_type == SP_AWAY)
        teleport_player(creature_ptr, 10 + (creature_ptr->concent * 2), TELEPORT_SPONTANEOUS);

    if (snipe_type == SP_FINAL) {
        msg_print(_("射撃の反動が体を襲った。", "The weapon's recoil stuns you. "));
        (void)set_slow(creature_ptr, creature_ptr->slow + randint0(7) + 7, false);
        (void)set_stun(creature_ptr, creature_ptr->stun + randint1(25));
    }
}
