#include "status/experience.h"
#include "player/player-status.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*
 * Gain experience
 */
void gain_exp_64(PlayerType *player_ptr, int32_t amount, uint32_t amount_frac)
{
    if (player_ptr->is_dead)
        return;
    if (player_ptr->prace == PlayerRaceType::ANDROID)
        return;

    s64b_add(&(player_ptr->exp), &(player_ptr->exp_frac), amount, amount_frac);

    if (player_ptr->exp < player_ptr->max_exp) {
        player_ptr->max_exp += amount / 5;
    }

    check_experience(player_ptr);
}

/*
 * Gain experience
 */
void gain_exp(PlayerType *player_ptr, int32_t amount) { gain_exp_64(player_ptr, amount, 0L); }

/*
 * Lose experience
 */
void lose_exp(PlayerType *player_ptr, int32_t amount)
{
    if (player_ptr->prace == PlayerRaceType::ANDROID)
        return;
    if (amount > player_ptr->exp)
        amount = player_ptr->exp;

    player_ptr->exp -= amount;

    check_experience(player_ptr);
}

/*
 * Restores any drained experience
 */
bool restore_level(PlayerType *player_ptr)
{
    if (player_ptr->exp < player_ptr->max_exp) {
        msg_print(_("経験値が戻ってきた気がする。", "You feel your experience returning."));
        player_ptr->exp = player_ptr->max_exp;
        check_experience(player_ptr);
        return true;
    }

    return false;
}

/*
 * Drain experience
 * If resisted to draining, return FALSE
 */
bool drain_exp(PlayerType *player_ptr, int32_t drain, int32_t slip, int hold_exp_prob)
{
    if (player_ptr->prace == PlayerRaceType::ANDROID)
        return false;

    if (player_ptr->hold_exp && (randint0(100) < hold_exp_prob)) {
        msg_print(_("しかし自己の経験値を守りきった！", "You keep hold of your experience!"));
        return false;
    }

    if (player_ptr->hold_exp) {
        msg_print(_("経験値を少し吸い取られた気がする！", "You feel your experience slipping away!"));
        lose_exp(player_ptr, slip);
    } else {
        msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away!"));
        lose_exp(player_ptr, drain);
    }

    return true;
}
