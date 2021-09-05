#include "effect/effect-player-oldies.h"
#include "effect/effect-player-util.h"
#include "game-option/birth-options.h"
#include "hpmp/hp-mp-processor.h"
#include "monster-race/race-indice-types.h"
#include "player/eldritch-horror.h"
#include "status/bad-status-setter.h"
#include "status/buff-setter.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

void effect_player_old_heal(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何らかの攻撃によって気分がよくなった。", "You are hit by something invigorating!"));

    (void)hp_player(target_ptr, ep_ptr->dam);
    ep_ptr->dam = 0;
}

void effect_player_old_speed(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何かで攻撃された！", "You are hit by something!"));

    (void)set_fast(target_ptr, target_ptr->fast + randint1(5), false);
    ep_ptr->dam = 0;
}

void effect_player_old_slow(player_type *target_ptr)
{
    if (target_ptr->blind)
        msg_print(_("何か遅いもので攻撃された！", "You are hit by something slow!"));

    (void)set_slow(target_ptr, target_ptr->slow + randint0(4) + 4, false);
}

void effect_player_old_sleep(player_type *target_ptr, effect_player_type *ep_ptr)
{
    if (target_ptr->free_act)
        return;

    if (target_ptr->blind)
        msg_print(_("眠ってしまった！", "You fall asleep!"));

    if (ironman_nightmare) {
        msg_print(_("恐ろしい光景が頭に浮かんできた。", "A horrible vision enters your mind."));

        /* Have some nightmares */
        sanity_blast(target_ptr, nullptr, false);
    }

    set_paralyzed(target_ptr, target_ptr->paralyzed + ep_ptr->dam);
    ep_ptr->dam = 0;
}
