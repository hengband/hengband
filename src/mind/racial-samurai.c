/*!
 * @brief 剣術家のレイシャルパワー処理
 * @date 2020/05/16
 * @author Hourier
 */

#include "system/angband.h"
#include "racial-samurai.h"
#include "cmd/cmd-pet.h"

bool concentration(player_type* creature_ptr)
{
    int max_csp = MAX(creature_ptr->msp * 4, creature_ptr->lev * 5 + 5);

    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return FALSE;
    }

    if (creature_ptr->special_defense & KATA_MASK) {
        msg_print(_("今は構えに集中している。", "You're already concentrating on your stance."));
        return FALSE;
    }

    msg_print(_("精神を集中して気合いを溜めた。", "You concentrate to charge your power."));

    creature_ptr->csp += creature_ptr->msp / 2;
    if (creature_ptr->csp >= max_csp) {
        creature_ptr->csp = max_csp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= PR_MANA;
    return TRUE;
}
