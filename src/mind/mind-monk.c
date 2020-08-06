#include "mind/mind-monk.h"
#include "action/action-limited.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "io/input-key-acceptor.h"
#include "mind/stances-table.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

/*!
 * @brief 修行僧の構え設定処理
 * @return 構えを変化させたらTRUE、構え不能かキャンセルしたらFALSEを返す。
 */
bool choose_monk_stance(player_type *creature_ptr)
{
    char choice;
    int new_kamae = 0;
    int i;
    char buf[80];

    if (cmd_limit_confused(creature_ptr))
        return FALSE;
    screen_save();
    prt(_(" a) 構えをとく", " a) No form"), 2, 20);

    for (i = 0; i < MAX_KAMAE; i++) {
        if (creature_ptr->lev >= monk_stances[i].min_level) {
            sprintf(buf, " %c) %-12s  %s", I2A(i + 1), monk_stances[i].desc, monk_stances[i].info);
            prt(buf, 3 + i, 20);
        }
    }

    prt("", 1, 0);
    prt(_("        どの構えをとりますか？", "        Choose Stance: "), 1, 14);

    while (TRUE) {
        choice = inkey();

        if (choice == ESCAPE) {
            screen_load();
            return FALSE;
        } else if ((choice == 'a') || (choice == 'A')) {
            if (creature_ptr->action == ACTION_KAMAE) {
                set_action(creature_ptr, ACTION_NONE);
            } else
                msg_print(_("もともと構えていない。", "You are not in a special stance."));
            screen_load();
            return TRUE;
        } else if ((choice == 'b') || (choice == 'B')) {
            new_kamae = 0;
            break;
        } else if (((choice == 'c') || (choice == 'C')) && (creature_ptr->lev > 29)) {
            new_kamae = 1;
            break;
        } else if (((choice == 'd') || (choice == 'D')) && (creature_ptr->lev > 34)) {
            new_kamae = 2;
            break;
        } else if (((choice == 'e') || (choice == 'E')) && (creature_ptr->lev > 39)) {
            new_kamae = 3;
            break;
        }
    }

    set_action(creature_ptr, ACTION_KAMAE);
    if (creature_ptr->special_defense & (KAMAE_GENBU << new_kamae)) {
        msg_print(_("構え直した。", "You reassume a stance."));
    } else {
        creature_ptr->special_defense &= ~(KAMAE_MASK);
        creature_ptr->update |= PU_BONUS;
        creature_ptr->redraw |= PR_STATE;
        msg_format(_("%sの構えをとった。", "You assume the %s stance."), monk_stances[new_kamae].desc);
        creature_ptr->special_defense |= (KAMAE_GENBU << new_kamae);
    }

    creature_ptr->redraw |= PR_STATE;
    screen_load();
    return TRUE;
}
