#include "mind/mind-warrior-mage.h"
#include "core/hp-mp-processor.h"
#include "core/player-redraw-types.h"
#include "player/player-damage.h"
#include "view/display-messages.h"

bool comvert_hp_to_mp(player_type *creature_ptr)
{
    int gain_sp = take_hit(creature_ptr, DAMAGE_USELIFE, creature_ptr->lev, _("ＨＰからＭＰへの無謀な変換", "thoughtless conversion from HP to SP"), -1) / 5;
    if (!gain_sp) {
        msg_print(_("変換に失敗した。", "You failed to convert."));
        creature_ptr->redraw |= (PR_HP | PR_MANA);
        return TRUE;
    }

    creature_ptr->csp += gain_sp;
    if (creature_ptr->csp > creature_ptr->msp) {
        creature_ptr->csp = creature_ptr->msp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= (PR_HP | PR_MANA);
    return TRUE;
}

bool comvert_mp_to_hp(player_type *creature_ptr)
{
    if (creature_ptr->csp >= creature_ptr->lev / 5) {
        creature_ptr->csp -= creature_ptr->lev / 5;
        hp_player(creature_ptr, creature_ptr->lev);
    } else {
        msg_print(_("変換に失敗した。", "You failed to convert."));
    }

    creature_ptr->redraw |= (PR_HP | PR_MANA);
    return TRUE;
}
