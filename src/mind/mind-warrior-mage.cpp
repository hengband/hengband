#include "mind/mind-warrior-mage.h"
#include "hpmp/hp-mp-processor.h"
#include "player/player-damage.h"
#include "system/deferred-update-requests.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

bool comvert_hp_to_mp(PlayerType *player_ptr)
{
    constexpr auto mes = _("ＨＰからＭＰへの無謀な変換", "thoughtless conversion from HP to SP");
    auto gain_sp = take_hit(player_ptr, DAMAGE_USELIFE, player_ptr->lev, mes) / 5;
    auto &dur = DeferredUpdateRequests::get_instance();
    if (!gain_sp) {
        msg_print(_("変換に失敗した。", "You failed to convert."));
        dur.request_hp_mp_redrawing();
        return true;
    }

    player_ptr->csp += gain_sp;
    if (player_ptr->csp > player_ptr->msp) {
        player_ptr->csp = player_ptr->msp;
        player_ptr->csp_frac = 0;
    }

    dur.request_hp_mp_redrawing();
    return true;
}

bool comvert_mp_to_hp(PlayerType *player_ptr)
{
    if (player_ptr->csp >= player_ptr->lev / 5) {
        player_ptr->csp -= player_ptr->lev / 5;
        hp_player(player_ptr, player_ptr->lev);
    } else {
        msg_print(_("変換に失敗した。", "You failed to convert."));
    }

    DeferredUpdateRequests::get_instance().request_hp_mp_redrawing();
    return true;
}
