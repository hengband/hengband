#include "status/experience.h"
#include "player/player-status.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*
 * Gain experience
 */
void gain_exp_64(player_type *creature_ptr, int32_t amount, uint32_t amount_frac)
{
    if (creature_ptr->is_dead)
        return;
    if (creature_ptr->prace == player_race_type::ANDROID)
        return;

    s64b_add(&(creature_ptr->exp), &(creature_ptr->exp_frac), amount, amount_frac);

    if (creature_ptr->exp < creature_ptr->max_exp) {
        creature_ptr->max_exp += amount / 5;
    }

    check_experience(creature_ptr);
}

/*
 * Gain experience
 */
void gain_exp(player_type *creature_ptr, int32_t amount) { gain_exp_64(creature_ptr, amount, 0L); }

/*
 * Lose experience
 */
void lose_exp(player_type *creature_ptr, int32_t amount)
{
    if (creature_ptr->prace == player_race_type::ANDROID)
        return;
    if (amount > creature_ptr->exp)
        amount = creature_ptr->exp;

    creature_ptr->exp -= amount;

    check_experience(creature_ptr);
}

/*
 * Restores any drained experience
 */
bool restore_level(player_type *creature_ptr)
{
    if (creature_ptr->exp < creature_ptr->max_exp) {
        msg_print(_("経験値が戻ってきた気がする。", "You feel your experience returning."));
        creature_ptr->exp = creature_ptr->max_exp;
        check_experience(creature_ptr);
        return true;
    }

    return false;
}

/*
 * Drain experience
 * If resisted to draining, return FALSE
 */
bool drain_exp(player_type *creature_ptr, int32_t drain, int32_t slip, int hold_exp_prob)
{
    if (creature_ptr->prace == player_race_type::ANDROID)
        return false;

    if (creature_ptr->hold_exp && (randint0(100) < hold_exp_prob)) {
        msg_print(_("しかし自己の経験値を守りきった！", "You keep hold of your experience!"));
        return false;
    }

    if (creature_ptr->hold_exp) {
        msg_print(_("経験値を少し吸い取られた気がする！", "You feel your experience slipping away!"));
        lose_exp(creature_ptr, slip);
    } else {
        msg_print(_("経験値が体から吸い取られた気がする！", "You feel your experience draining away!"));
        lose_exp(creature_ptr, drain);
    }

    return true;
}
