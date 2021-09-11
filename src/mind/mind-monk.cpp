#include "mind/mind-monk.h"
#include "action/action-limited.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "io/input-key-acceptor.h"
#include "mind/stances-table.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

static void set_stance(player_type *player_ptr, const int new_stance)
{
    set_action(player_ptr, ACTION_KAMAE);
    if (player_ptr->special_defense & (KAMAE_GENBU << new_stance)) {
        msg_print(_("構え直した。", "You reassume a stance."));
        return;
    }

    player_ptr->special_defense &= ~(KAMAE_MASK);
    player_ptr->update |= PU_BONUS;
    player_ptr->redraw |= PR_STATE;
    msg_format(_("%sの構えをとった。", "You assume the %s stance."), monk_stances[new_stance].desc);
    player_ptr->special_defense |= (KAMAE_GENBU << new_stance);
}

/*!
 * @brief 修行僧の構え設定処理
 * @return 構えを変化させたらTRUE、構え不能かキャンセルしたらFALSEを返す。
 */
bool choose_monk_stance(player_type *player_ptr)
{
    if (cmd_limit_confused(player_ptr))
        return false;

    screen_save();
    prt(_(" a) 構えをとく", " a) No form"), 2, 20);
    for (int i = 0; i < MAX_KAMAE; i++) {
        if (player_ptr->lev >= monk_stances[i].min_level) {
            char buf[80];
            sprintf(buf, " %c) %-12s  %s", I2A(i + 1), monk_stances[i].desc, monk_stances[i].info);
            prt(buf, 3 + i, 20);
        }
    }

    prt("", 1, 0);
    prt(_("        どの構えをとりますか？", "        Choose Stance: "), 1, 14);

    int new_stance = 0;
    while (true) {
        char choice = inkey();
        if (choice == ESCAPE) {
            screen_load();
            return false;
        }
        
        if ((choice == 'a') || (choice == 'A')) {
            if (player_ptr->action == ACTION_KAMAE) {
                set_action(player_ptr, ACTION_NONE);
            } else
                msg_print(_("もともと構えていない。", "You are not in a special stance."));
            screen_load();
            return true;
        }
        
        if ((choice == 'b') || (choice == 'B')) {
            new_stance = 0;
            break;
        } else if (((choice == 'c') || (choice == 'C')) && (player_ptr->lev > 29)) {
            new_stance = 1;
            break;
        } else if (((choice == 'd') || (choice == 'D')) && (player_ptr->lev > 34)) {
            new_stance = 2;
            break;
        } else if (((choice == 'e') || (choice == 'E')) && (player_ptr->lev > 39)) {
            new_stance = 3;
            break;
        }
    }

    set_stance(player_ptr, new_stance);
    player_ptr->redraw |= PR_STATE;
    screen_load();
    return true;
}
