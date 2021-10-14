#include "effect/effect-player-curse.h"
#include "blue-magic/blue-magic-checker.h"
#include "effect/effect-player-util.h"
#include "mind/mind-mirror-master.h"
#include "monster-race/race-indice-types.h"
#include "object-enchant/object-curse.h"
#include "player/player-damage.h"
#include "status/bad-status-setter.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"

void effect_player_curse_1(player_type *player_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < player_ptr->skill_sav) && !check_multishadow(player_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else {
        if (!check_multishadow(player_ptr))
            curse_equipment(player_ptr, 15, 0);
        ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    }
}

void effect_player_curse_2(player_type *player_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < player_ptr->skill_sav) && !check_multishadow(player_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else {
        if (!check_multishadow(player_ptr))
            curse_equipment(player_ptr, 25, std::min(ep_ptr->rlev / 2 - 15, 5));
        ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    }
}

void effect_player_curse_3(player_type *player_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < player_ptr->skill_sav) && !check_multishadow(player_ptr)) {
        msg_print(_("しかし効力を跳ね返した！", "You resist the effects!"));
    } else {
        if (!check_multishadow(player_ptr))
            curse_equipment(player_ptr, 33, std::min(ep_ptr->rlev / 2 - 15, 15));
        ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    }
}

void effect_player_curse_4(player_type *player_ptr, effect_player_type *ep_ptr)
{
    if ((randint0(100 + ep_ptr->rlev / 2) < player_ptr->skill_sav) && (ep_ptr->m_ptr->r_idx != MON_KENSHIROU) && !check_multishadow(player_ptr)) {
        msg_print(_("しかし秘孔を跳ね返した！", "You resist the effects!"));
        return;
    }

    ep_ptr->get_damage = take_hit(player_ptr, DAMAGE_ATTACK, ep_ptr->dam, ep_ptr->killer);
    if (!check_multishadow(player_ptr)) {
        (void)BadStatusSetter(player_ptr).mod_cut(damroll(10, 10));
    }
}
