#include "system/angband.h"
#include "cmd-action/cmd-pet.h"
#include "mind/racial-mirror-master.h"
#include "world/world.h"

/*
 * @brief Multishadow effects is determined by turn
 */
bool check_multishadow(player_type *creature_ptr)
{
    return (creature_ptr->multishadow != 0) && ((current_world_ptr->game_turn & 1) != 0);
}

/*!
 * 静水
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return ペットを操っている場合を除きTRUE
 */
bool mirror_concentration(player_type *creature_ptr)
{
    if (total_friends) {
        msg_print(_("今はペットを操ることに集中していないと。", "Your pets demand all of your attention."));
        return FALSE;
    }

    if (!is_mirror_grid(&creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x])) {
        msg_print(_("鏡の上でないと集中できない！", "There's no mirror here!"));
        return TRUE;
    }

    msg_print(_("少し頭がハッキリした。", "You feel your head clear a little."));

    creature_ptr->csp += (5 + creature_ptr->lev * creature_ptr->lev / 100);
    if (creature_ptr->csp >= creature_ptr->msp) {
        creature_ptr->csp = creature_ptr->msp;
        creature_ptr->csp_frac = 0;
    }

    creature_ptr->redraw |= PR_MANA;
    return TRUE;
}
