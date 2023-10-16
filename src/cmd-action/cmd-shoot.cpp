#include "cmd-action/cmd-shoot.h"
#include "combat/shoot.h"
#include "floor/floor-object.h"
#include "inventory/inventory-slot-types.h"
#include "mind/snipe-types.h"
#include "object/item-tester-hooker.h"
#include "object/item-use-flags.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player-info/sniper-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-teleport.h"
#include "status/action-setter.h"
#include "status/bad-status-setter.h"
#include "sv-definition/sv-bow-types.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "view/display-messages.h"

/*!
 * @brief 射撃処理のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param snipe_type スナイパーの射撃術の種類
 */
void do_cmd_fire(PlayerType *player_ptr, SPELL_IDX snipe_type)
{
    if (player_ptr->wild_mode) {
        return;
    }

    player_ptr->is_fired = false;
    auto *item_ptr = &player_ptr->inventory_list[INVEN_BOW];
    const auto tval = item_ptr->bi_key.tval();
    if (tval == ItemKindType::NONE) {
        msg_print(_("射撃用の武器を持っていない。", "You have nothing to fire with."));
        flush();
        return;
    }

    const auto sval = item_ptr->bi_key.sval();
    if (sval == SV_CRIMSON) {
        msg_print(_("この武器は発動して使うもののようだ。", "It's already activated."));
        flush();
        return;
    }

    if (sval == SV_HARP) {
        msg_print(_("この武器で射撃はできない。", "It's not for firing."));
        flush();
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
    constexpr auto q = _("どれを撃ちますか? ", "Fire which item? ");
    constexpr auto s = _("発射されるアイテムがありません。", "You have nothing to fire.");
    short i_idx;
    const auto *ammo_ptr = choose_object(player_ptr, &i_idx, q, s, USE_INVEN | USE_FLOOR, TvalItemTester(player_ptr->tval_ammo));
    if (!ammo_ptr) {
        flush();
        return;
    }

    exe_fire(player_ptr, i_idx, item_ptr, snipe_type);
    if (!player_ptr->is_fired || !PlayerClass(player_ptr).equals(PlayerClassType::SNIPER)) {
        return;
    }

    if (snipe_type == SP_AWAY) {
        auto sniper_data = PlayerClass(player_ptr).get_specific_data<SniperData>();
        teleport_player(player_ptr, 10 + (sniper_data->concent * 2), TELEPORT_SPONTANEOUS);
    }

    auto effects = player_ptr->effects();
    if (snipe_type == SP_FINAL) {
        msg_print(_("射撃の反動が体を襲った。", "The weapon's recoil stuns you. "));
        BadStatusSetter bss(player_ptr);
        (void)bss.mod_deceleration(randint0(7) + 7, false);
        (void)bss.mod_stun(randint1(25));
    }
}
