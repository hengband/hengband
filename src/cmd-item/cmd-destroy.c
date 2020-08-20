#include "cmd-item/cmd-destroy.h"
#include "autopick/autopick-registry.h"
#include "autopick/autopick.h"
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
#include "object/object-generator.h"
#include "object/object-stack.h"
#include "object/object-value.h"
#include "player/attack-defense-types.h"
#include "player/avatar.h"
#include "player/special-defense-types.h"
#include "racial/racial-android.h"
#include "realm/realm-names-table.h"
#include "status/action-setter.h"
#include "status/experience.h"
#include "system/object-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief アイテムを破壊するコマンドのメインルーチン / Destroy an item
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void do_cmd_destroy(player_type *creature_ptr)
{
    OBJECT_IDX item;
    QUANTITY amt = 1;
    QUANTITY old_number;
    bool force = FALSE;
    object_type *o_ptr;
    object_type forge;
    object_type *q_ptr = &forge;
    GAME_TEXT o_name[MAX_NLEN];
    char out_val[MAX_NLEN + 40];
    if (creature_ptr->special_defense & KATA_MUSOU)
        set_action(creature_ptr, ACTION_NONE);

    if (command_arg > 0)
        force = TRUE;

    concptr q = _("どのアイテムを壊しますか? ", "Destroy which item? ");
    concptr s = _("壊せるアイテムを持っていない。", "You have nothing to destroy.");
    o_ptr = choose_object(creature_ptr, &item, q, s, USE_INVEN | USE_FLOOR, 0);
    if (!o_ptr)
        return;

    if (!force && (confirm_destroy || (object_value(creature_ptr, o_ptr) > 0))) {
        describe_flavor(creature_ptr, o_name, o_ptr, OD_OMIT_PREFIX);
        sprintf(out_val, _("本当に%sを壊しますか? [y/n/Auto]", "Really destroy %s? [y/n/Auto]"), o_name);
        msg_print(NULL);
        message_add(out_val);
        creature_ptr->window |= PW_MESSAGE;
        handle_stuff(creature_ptr);
        while (TRUE) {
            prt(out_val, 0, 0);
            char i = inkey();
            prt("", 0, 0);
            if (i == 'y' || i == 'Y')
                break;

            if (i == ESCAPE || i == 'n' || i == 'N')
                return;

            if (i == 'A') {
                if (autopick_autoregister(creature_ptr, o_ptr))
                    autopick_alter_item(creature_ptr, item, TRUE);

                return;
            }
        }
    }

    if (o_ptr->number > 1) {
        amt = get_quantity(NULL, o_ptr->number);
        if (amt <= 0)
            return;
    }

    old_number = o_ptr->number;
    o_ptr->number = amt;
    describe_flavor(creature_ptr, o_name, o_ptr, 0);
    o_ptr->number = old_number;
    take_turn(creature_ptr, 100);
    if (!can_player_destroy_object(creature_ptr, o_ptr)) {
        free_turn(creature_ptr);
        msg_format(_("%sは破壊不可能だ。", "You cannot destroy %s."), o_name);
        return;
    }

    object_copy(q_ptr, o_ptr);
    msg_format(_("%sを壊した。", "You destroy %s."), o_name);
    sound(SOUND_DESTITEM);
    reduce_charges(o_ptr, amt);
    vary_item(creature_ptr, item, -amt);
    if (item_tester_high_level_book(q_ptr)) {
        bool gain_expr = FALSE;
        if (creature_ptr->prace == RACE_ANDROID) {
        } else if ((creature_ptr->pclass == CLASS_WARRIOR) || (creature_ptr->pclass == CLASS_BERSERKER)) {
            gain_expr = TRUE;
        } else if (creature_ptr->pclass == CLASS_PALADIN) {
            if (is_good_realm(creature_ptr->realm1)) {
                if (!is_good_realm(tval2realm(q_ptr->tval)))
                    gain_expr = TRUE;
            } else {
                if (is_good_realm(tval2realm(q_ptr->tval)))
                    gain_expr = TRUE;
            }
        }

        if (gain_expr && (creature_ptr->exp < PY_MAX_EXP)) {
            s32b tester_exp = creature_ptr->max_exp / 20;
            if (tester_exp > 10000)
                tester_exp = 10000;

            if (q_ptr->sval < 3)
                tester_exp /= 4;

            if (tester_exp < 1)
                tester_exp = 1;

            msg_print(_("更に経験を積んだような気がする。", "You feel more experienced."));
            gain_exp(creature_ptr, tester_exp * amt);
        }

        if (item_tester_high_level_book(q_ptr) && q_ptr->tval == TV_LIFE_BOOK) {
            chg_virtue(creature_ptr, V_UNLIFE, 1);
            chg_virtue(creature_ptr, V_VITALITY, -1);
        } else if (item_tester_high_level_book(q_ptr) && q_ptr->tval == TV_DEATH_BOOK) {
            chg_virtue(creature_ptr, V_UNLIFE, -1);
            chg_virtue(creature_ptr, V_VITALITY, 1);
        }

        if (q_ptr->to_a || q_ptr->to_h || q_ptr->to_d)
            chg_virtue(creature_ptr, V_ENCHANT, -1);

        if (object_value_real(creature_ptr, q_ptr) > 30000)
            chg_virtue(creature_ptr, V_SACRIFICE, 2);
        else if (object_value_real(creature_ptr, q_ptr) > 10000)
            chg_virtue(creature_ptr, V_SACRIFICE, 1);
    }

    if (q_ptr->to_a != 0 || q_ptr->to_d != 0 || q_ptr->to_h != 0)
        chg_virtue(creature_ptr, V_HARMONY, 1);

    if (item >= INVEN_RARM)
        calc_android_exp(creature_ptr);
}
