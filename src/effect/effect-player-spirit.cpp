#include "effect/effect-player-spirit.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "effect/effect-player-util.h"
#include "mind/mind-mirror-master.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

void effect_player_drain_mana(PlayerType *player_ptr, effect_player_type *ep_ptr)
{
    if (check_multishadow(player_ptr)) {
        msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
        ep_ptr->dam = 0;
        return;
    }

    if (player_ptr->csp == 0) {
        ep_ptr->dam = 0;
        return;
    }

    if (ep_ptr->who > 0)
        msg_format(_("%^sに精神エネルギーを吸い取られてしまった！", "%^s draws psychic energy from you!"), ep_ptr->m_name);
    else
        msg_print(_("精神エネルギーを吸い取られてしまった！", "Your psychic energy is drained!"));

    if (ep_ptr->dam >= player_ptr->csp) {
        ep_ptr->dam = player_ptr->csp;
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
    } else {
        player_ptr->csp -= ep_ptr->dam;
    }

    player_ptr->redraw |= (PR_MANA);
    player_ptr->window_flags |= (PW_PLAYER | PW_SPELL);

    if ((ep_ptr->who <= 0) || (ep_ptr->m_ptr->hp >= ep_ptr->m_ptr->maxhp)) {
        ep_ptr->dam = 0;
        return;
    }

    ep_ptr->m_ptr->hp += ep_ptr->dam;
    if (ep_ptr->m_ptr->hp > ep_ptr->m_ptr->maxhp)
        ep_ptr->m_ptr->hp = ep_ptr->m_ptr->maxhp;

    if (player_ptr->health_who == ep_ptr->who)
        player_ptr->redraw |= (PR_HEALTH);
    if (player_ptr->riding == ep_ptr->who)
        player_ptr->redraw |= (PR_UHEALTH);

    if (ep_ptr->m_ptr->ml) {
        msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), ep_ptr->m_name);
    }

    ep_ptr->dam = 0;
}

void effect_player_mind_blast(PlayerType *player_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < std::max<short>(5, player_ptr->skill_sav)) && !check_multishadow(player_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        return;
    }

    if (check_multishadow(player_ptr)) {
        ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
        return;
    }

    msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psionic energy."));
    BadStatusSetter bss(player_ptr);
    if (!has_resist_conf(player_ptr)) {
        (void)bss.mod_confusion(randint0(4) + 4);
    }

    if (!has_resist_chaos(player_ptr) && one_in_(3)) {
        (void)bss.mod_hallucination(randint0(250) + 150);
    }

    player_ptr->csp -= 50;
    if (player_ptr->csp < 0) {
        player_ptr->csp = 0;
        player_ptr->csp_frac = 0;
    }

    player_ptr->redraw |= PR_MANA;
    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
}

void effect_player_brain_smash(PlayerType *player_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < std::max<short>(5, player_ptr->skill_sav)) && !check_multishadow(player_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        return;
    }

    if (!check_multishadow(player_ptr)) {
        msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psionic energy."));

        player_ptr->csp -= 100;
        if (player_ptr->csp < 0) {
            player_ptr->csp = 0;
            player_ptr->csp_frac = 0;
        }
        player_ptr->redraw |= PR_MANA;
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    if (check_multishadow(player_ptr))
        return;

    BadStatusSetter bss(player_ptr);
    if (!has_resist_blind(player_ptr)) {
        (void)bss.mod_blindness(8 + randint0(8));
    }

    if (!has_resist_conf(player_ptr)) {
        (void)bss.mod_confusion(randint0(4) + 4);
    }

    if (!player_ptr->free_act) {
        (void)bss.mod_paralysis(randint0(4) + 4);
    }

    (void)bss.mod_slowness(randint0(4) + 4, false);

    while (randint0(100 + ep_ptr->rlev / 2) > (std::max<short>(5, player_ptr->skill_sav)))
        (void)do_dec_stat(player_ptr, A_INT);
    while (randint0(100 + ep_ptr->rlev / 2) > (std::max<short>(5, player_ptr->skill_sav)))
        (void)do_dec_stat(player_ptr, A_WIS);

    if (!has_resist_chaos(player_ptr)) {
        (void)bss.mod_hallucination(randint0(250) + 150);
    }
}
