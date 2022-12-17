#include "cmd-item/cmd-destroy.h"
#include "autopick/autopick-registry.h"
#include "autopick/autopick.h"
#include "avatar/avatar.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/floor-object.h"
#include "game-option/input-options.h"
#include "inventory/inventory-object.h"
#include "inventory/inventory-slot-types.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-hook/hook-expendable.h"
#include "object-hook/hook-magic.h"
#include "object/item-use-flags.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "player-base/player-class.h"
#include "player-base/player-race.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "realm/realm-names-table.h"
#include "status/action-setter.h"
#include "status/experience.h"
#include "system/baseitem-info.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

struct destroy_type {
    OBJECT_IDX item;
    QUANTITY amt;
    QUANTITY old_number;
    bool force;
    ItemEntity *o_ptr;
    ItemEntity *q_ptr;
    GAME_TEXT o_name[MAX_NLEN];
    char out_val[MAX_NLEN + 40];
};

static destroy_type *initialize_destroy_type(destroy_type *destroy_ptr, ItemEntity *o_ptr)
{
    destroy_ptr->amt = 1;
    destroy_ptr->force = false;
    destroy_ptr->q_ptr = o_ptr;
    return destroy_ptr;
}

static bool check_destory_item(PlayerType *player_ptr, destroy_type *destroy_ptr)
{
    if (destroy_ptr->force || (!confirm_destroy && (destroy_ptr->o_ptr->get_price() <= 0))) {
        return true;
    }

    describe_flavor(player_ptr, destroy_ptr->o_name, destroy_ptr->o_ptr, OD_OMIT_PREFIX);
    strnfmt(destroy_ptr->out_val, sizeof(destroy_ptr->out_val), _("本当に%sを壊しますか? [y/n/Auto]", "Really destroy %s? [y/n/Auto]"), destroy_ptr->o_name);
    msg_print(nullptr);
    message_add(destroy_ptr->out_val);
    player_ptr->window_flags |= PW_MESSAGE;
    handle_stuff(player_ptr);
    while (true) {
        prt(destroy_ptr->out_val, 0, 0);
        char i = inkey();
        prt("", 0, 0);
        if (i == 'y' || i == 'Y') {
            return true;
        }

        if (i == ESCAPE || i == 'n' || i == 'N') {
            return false;
        }

        if (i != 'A') {
            continue;
        }

        if (autopick_autoregister(player_ptr, destroy_ptr->o_ptr)) {
            autopick_alter_item(player_ptr, destroy_ptr->item, true);
        }

        return false;
    }
}

static bool select_destroying_item(PlayerType *player_ptr, destroy_type *destroy_ptr)
{
    concptr q = _("どのアイテムを壊しますか? ", "Destroy which item? ");
    concptr s = _("壊せるアイテムを持っていない。", "You have nothing to destroy.");
    destroy_ptr->o_ptr = choose_object(player_ptr, &destroy_ptr->item, q, s, USE_INVEN | USE_FLOOR);
    if (destroy_ptr->o_ptr == nullptr) {
        return false;
    }

    if (!check_destory_item(player_ptr, destroy_ptr)) {
        return false;
    }

    if (destroy_ptr->o_ptr->number <= 1) {
        return true;
    }

    destroy_ptr->amt = get_quantity(nullptr, destroy_ptr->o_ptr->number);
    return destroy_ptr->amt > 0;
}

/*!
 * @brief 一部職業で高位魔法書の破壊による経験値上昇の判定
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param destory_ptr アイテム破壊構造体への参照ポインタ
 * return 魔法書の破壊によって経験値が入るならばTRUE
 */
static bool decide_magic_book_exp(PlayerType *player_ptr, destroy_type *destroy_ptr)
{
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        return false;
    }

    PlayerClass pc(player_ptr);
    const auto tval = destroy_ptr->o_ptr->bi_key.tval();
    if (pc.equals(PlayerClassType::WARRIOR) || pc.equals(PlayerClassType::BERSERKER)) {
        return tval != ItemKindType::HISSATSU_BOOK;
    }

    if (!pc.equals(PlayerClassType::PALADIN)) {
        return false;
    }

    auto is_good_magic_realm = (tval == ItemKindType::LIFE_BOOK) || (tval == ItemKindType::CRUSADE_BOOK);
    if (is_good_realm(player_ptr->realm1)) {
        return !is_good_magic_realm;
    } else {
        return is_good_magic_realm;
    }
}

static void gain_exp_by_destroying_magic_book(PlayerType *player_ptr, destroy_type *destroy_ptr)
{
    const auto gain_expr = decide_magic_book_exp(player_ptr, destroy_ptr);
    if (!gain_expr || (player_ptr->exp >= PY_MAX_EXP)) {
        return;
    }

    auto tester_exp = player_ptr->max_exp / 20;
    if (tester_exp > 10000) {
        tester_exp = 10000;
    }

    if (destroy_ptr->q_ptr->bi_key.sval() < 3) {
        tester_exp /= 4;
    }

    if (tester_exp < 1) {
        tester_exp = 1;
    }

    msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
    gain_exp(player_ptr, tester_exp * destroy_ptr->amt);
}

static void process_destroy_magic_book(PlayerType *player_ptr, destroy_type *destroy_ptr)
{
    const auto *q_ptr = destroy_ptr->q_ptr;
    const BaseitemKey &bi_key = q_ptr->bi_key;
    if (!bi_key.is_high_level_book()) {
        return;
    }

    const auto tval = bi_key.tval();
    gain_exp_by_destroying_magic_book(player_ptr, destroy_ptr);
    if (tval == ItemKindType::LIFE_BOOK) {
        chg_virtue(player_ptr, V_UNLIFE, 1);
        chg_virtue(player_ptr, V_VITALITY, -1);
    } else if (tval == ItemKindType::DEATH_BOOK) {
        chg_virtue(player_ptr, V_UNLIFE, -1);
        chg_virtue(player_ptr, V_VITALITY, 1);
    }

    if ((destroy_ptr->q_ptr->to_a != 0) || (destroy_ptr->q_ptr->to_h != 0) || (destroy_ptr->q_ptr->to_d != 0)) {
        chg_virtue(player_ptr, V_ENCHANT, -1);
    }

    if (object_value_real(destroy_ptr->q_ptr) > 30000) {
        chg_virtue(player_ptr, V_SACRIFICE, 2);
    } else if (object_value_real(destroy_ptr->q_ptr) > 10000) {
        chg_virtue(player_ptr, V_SACRIFICE, 1);
    }
}

static void exe_destroy_item(PlayerType *player_ptr, destroy_type *destroy_ptr)
{
    destroy_ptr->q_ptr->copy_from(destroy_ptr->o_ptr);
    msg_format(_("%sを壊した。", "You destroy %s."), destroy_ptr->o_name);
    sound(SOUND_DESTITEM);
    reduce_charges(destroy_ptr->o_ptr, destroy_ptr->amt);
    vary_item(player_ptr, destroy_ptr->item, -destroy_ptr->amt);
    process_destroy_magic_book(player_ptr, destroy_ptr);
    if ((destroy_ptr->q_ptr->to_a != 0) || (destroy_ptr->q_ptr->to_d != 0) || (destroy_ptr->q_ptr->to_h != 0)) {
        chg_virtue(player_ptr, V_HARMONY, 1);
    }

    if (destroy_ptr->item >= INVEN_MAIN_HAND) {
        calc_android_exp(player_ptr);
    }
}

/*!
 * @brief アイテムを破壊するコマンドのメインルーチン / Destroy an item
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_destroy(PlayerType *player_ptr)
{
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    ItemEntity forge;
    destroy_type tmp_destroy;
    destroy_type *destroy_ptr = initialize_destroy_type(&tmp_destroy, &forge);
    if (command_arg > 0) {
        destroy_ptr->force = true;
    }

    if (!select_destroying_item(player_ptr, destroy_ptr)) {
        return;
    }

    destroy_ptr->old_number = destroy_ptr->o_ptr->number;
    destroy_ptr->o_ptr->number = destroy_ptr->amt;
    describe_flavor(player_ptr, destroy_ptr->o_name, destroy_ptr->o_ptr, 0);
    destroy_ptr->o_ptr->number = destroy_ptr->old_number;
    PlayerEnergy energy(player_ptr);
    energy.set_player_turn_energy(100);
    if (!can_player_destroy_object(player_ptr, destroy_ptr->o_ptr)) {
        energy.reset_player_turn();
        msg_format(_("%sは破壊不可能だ。", "You cannot destroy %s."), destroy_ptr->o_name);
        return;
    }

    exe_destroy_item(player_ptr, destroy_ptr);
}
