#include "mind/mind-warrior-mage.h"
#include "core/player-redraw-types.h"
#include "hpmp/hp-mp-processor.h"
#include "player/player-damage.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool comvert_hp_to_mp(player_type *player_ptr)
{
    int gain_sp = take_hit(player_ptr, DAMAGE_USELIFE, player_ptr->lev, _("ＨＰからＭＰへの無謀な変換", "thoughtless conversion from HP to SP")) / 5;
    if (!gain_sp) {
        msg_print(_("変換に失敗した。", "You failed to convert."));
        player_ptr->redraw |= (PR_HP | PR_MANA);
        return true;
    }

    player_ptr->csp += gain_sp;
    if (player_ptr->csp > player_ptr->msp) {
        player_ptr->csp = player_ptr->msp;
        player_ptr->csp_frac = 0;
    }

    player_ptr->redraw |= (PR_HP | PR_MANA);
    return true;
}

bool comvert_mp_to_hp(player_type *player_ptr)
{
    if (player_ptr->csp >= player_ptr->lev / 5) {
        player_ptr->csp -= player_ptr->lev / 5;
        hp_player(player_ptr, player_ptr->lev);
    } else {
        msg_print(_("変換に失敗した。", "You failed to convert."));
    }

    player_ptr->redraw |= (PR_HP | PR_MANA);
    return true;
}
