#include "effect/effect-player-spirit.h"
#include "blue-magic/blue-magic-checker.h"
#include "core/player-redraw-types.h"
#include "core/window-redrawer.h"
#include "mind/mind-mirror-master.h"
#include "player/player-damage.h"
#include "player/player-status-flags.h"
#include "status/bad-status-setter.h"
#include "status/base-status.h"
#include "view/display-messages.h"
#include "world/world.h"

void effect_player_drain_mana(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (check_multishadow(target_ptr)) {
        msg_print(_("攻撃は幻影に命中し、あなたには届かなかった。", "The attack hits Shadow, but you are unharmed!"));
        ep_ptr->dam = 0;
        return;
    }

    if (target_ptr->csp == 0) {
        ep_ptr->dam = 0;
        return;
    }

    if (ep_ptr->who > 0)
        msg_format(_("%^sに精神エネルギーを吸い取られてしまった！", "%^s draws psychic energy from you!"), ep_ptr->m_name);
    else
        msg_print(_("精神エネルギーを吸い取られてしまった！", "Your psychic energy is drawn!"));

    if (ep_ptr->dam >= target_ptr->csp) {
        ep_ptr->dam = target_ptr->csp;
        target_ptr->csp = 0;
        target_ptr->csp_frac = 0;
    } else {
        target_ptr->csp -= ep_ptr->dam;
    }

    learn_spell(target_ptr, ep_ptr->monspell);
    target_ptr->redraw |= (PR_MANA);
    target_ptr->window |= (PW_PLAYER | PW_SPELL);

    if ((ep_ptr->who <= 0) || (ep_ptr->m_ptr->hp >= ep_ptr->m_ptr->maxhp)) {
        ep_ptr->dam = 0;
        return;
    }

    ep_ptr->m_ptr->hp += ep_ptr->dam;
    if (ep_ptr->m_ptr->hp > ep_ptr->m_ptr->maxhp)
        ep_ptr->m_ptr->hp = ep_ptr->m_ptr->maxhp;

    if (target_ptr->health_who == ep_ptr->who)
        target_ptr->redraw |= (PR_HEALTH);
    if (target_ptr->riding == ep_ptr->who)
        target_ptr->redraw |= (PR_UHEALTH);

    if (ep_ptr->m_ptr->ml) {
        msg_format(_("%^sは気分が良さそうだ。", "%^s appears healthier."), ep_ptr->m_name);
    }

    ep_ptr->dam = 0;
}

void effect_player_mind_blast(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < MAX(5, target_ptr->skill_sav)) && !check_multishadow(target_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        learn_spell(target_ptr, ep_ptr->monspell);
        return;
    }

    if (check_multishadow(target_ptr)) {
        ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
        return;
    }

    msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psionic energy."));
    if (!has_resist_conf(target_ptr)) {
        (void)set_confused(target_ptr, target_ptr->confused + randint0(4) + 4);
    }

    if (!has_resist_chaos(target_ptr) && one_in_(3)) {
        (void)set_image(target_ptr, target_ptr->image + randint0(250) + 150);
    }

    target_ptr->csp -= 50;
    if (target_ptr->csp < 0) {
        target_ptr->csp = 0;
        target_ptr->csp_frac = 0;
    }

    target_ptr->redraw |= PR_MANA;
    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
}

void effect_player_brain_smash(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < MAX(5, target_ptr->skill_sav)) && !check_multishadow(target_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
        learn_spell(target_ptr, ep_ptr->monspell);
        return;
    }

    if (!check_multishadow(target_ptr)) {
        msg_print(_("霊的エネルギーで精神が攻撃された。", "Your mind is blasted by psionic energy."));

        target_ptr->csp -= 100;
        if (target_ptr->csp < 0) {
            target_ptr->csp = 0;
            target_ptr->csp_frac = 0;
        }
        target_ptr->redraw |= PR_MANA;
    }

    ep_ptr->get_damage = take_hit(target_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer, ep_ptr->monspell);
    if (check_multishadow(target_ptr))
        return;

    if (!has_resist_blind(target_ptr)) {
        (void)set_blind(target_ptr, target_ptr->blind + 8 + randint0(8));
    }

    if (!has_resist_conf(target_ptr)) {
        (void)set_confused(target_ptr, target_ptr->confused + randint0(4) + 4);
    }

    if (!target_ptr->free_act) {
        (void)set_paralyzed(target_ptr, target_ptr->paralyzed + randint0(4) + 4);
    }

    (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, FALSE);

    while (randint0(100 + ep_ptr->rlev / 2) > (MAX(5, target_ptr->skill_sav)))
        (void)do_dec_stat(target_ptr, A_INT);
    while (randint0(100 + ep_ptr->rlev / 2) > (MAX(5, target_ptr->skill_sav)))
        (void)do_dec_stat(target_ptr, A_WIS);

    if (!has_resist_chaos(target_ptr)) {
        (void)set_image(target_ptr, target_ptr->image + randint0(250) + 150);
    }
}
