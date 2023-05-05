#include "mind/mind-monk.h"
#include "action/action-limited.h"
#include "core/player-redraw-types.h"
#include "io/input-key-acceptor.h"
#include "mind/stances-table.h"
#include "player-base/player-class.h"
#include "player-info/monk-data-type.h"
#include "player/attack-defense-types.h"
#include "player/special-defense-types.h"
#include "status/action-setter.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/screen-processor.h"
#include "term/z-form.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"

static void set_stance(PlayerType *player_ptr, const MonkStanceType new_stance)
{
    set_action(player_ptr, ACTION_MONK_STANCE);
    PlayerClass pc(player_ptr);
    if (pc.monk_stance_is(new_stance)) {
        msg_print(_("構え直した。", "You reassume a stance."));
        return;
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRedrawingFlag::BONUS);
    player_ptr->redraw |= PR_ACTION;
    msg_format(_("%sの構えをとった。", "You assume the %s stance."), monk_stances[enum2i(new_stance) - 1].desc);
    pc.set_monk_stance(new_stance);
}

/*!
 * @brief 修行僧の構え設定処理
 * @return 構えを変化させたらTRUE、構え不能かキャンセルしたらFALSEを返す。
 */
bool choose_monk_stance(PlayerType *player_ptr)
{
    if (cmd_limit_confused(player_ptr)) {
        return false;
    }

    screen_save();
    prt(_(" a) 構えをとく", " a) No form"), 2, 20);
    for (auto i = 0U; i < monk_stances.size(); i++) {
        if (player_ptr->lev >= monk_stances[i].min_level) {
            char buf[80];
            strnfmt(buf, sizeof(buf), " %c) %-12s  %s", I2A(i + 1), monk_stances[i].desc, monk_stances[i].info);
            prt(buf, 3 + i, 20);
        }
    }

    prt("", 1, 0);
    prt(_("        どの構えをとりますか？", "        Choose Stance: "), 1, 14);

    auto new_stance = MonkStanceType::NONE;
    while (true) {
        char choice = inkey();
        if (choice == ESCAPE) {
            screen_load();
            return false;
        }

        if ((choice == 'a') || (choice == 'A')) {
            if (player_ptr->action == ACTION_MONK_STANCE) {
                set_action(player_ptr, ACTION_NONE);
            } else {
                msg_print(_("もともと構えていない。", "You are not in a special stance."));
            }
            screen_load();
            return true;
        }

        if ((choice == 'b') || (choice == 'B')) {
            new_stance = MonkStanceType::GENBU;
            break;
        } else if (((choice == 'c') || (choice == 'C')) && (player_ptr->lev > 29)) {
            new_stance = MonkStanceType::BYAKKO;
            break;
        } else if (((choice == 'd') || (choice == 'D')) && (player_ptr->lev > 34)) {
            new_stance = MonkStanceType::SEIRYU;
            break;
        } else if (((choice == 'e') || (choice == 'E')) && (player_ptr->lev > 39)) {
            new_stance = MonkStanceType::SUZAKU;
            break;
        }
    }

    set_stance(player_ptr, new_stance);
    player_ptr->redraw |= PR_ACTION;
    screen_load();
    return true;
}
