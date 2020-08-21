#include "player/digestion-processor.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "floor/wild.h"
#include "game-option/disturbance-options.h"
#include "object-enchant/trc-types.h"
#include "player-info/avatar.h"
#include "player/player-damage.h"
#include "player/special-defense-types.h"
#include "status/bad-status-setter.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーの腹を減らす
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void starve_player(player_type *creature_ptr)
{
    if (creature_ptr->phase_out)
        return;

    if (creature_ptr->food >= PY_FOOD_MAX) {
        (void)set_food(creature_ptr, creature_ptr->food - 100);
    } else if (!(current_world_ptr->game_turn % (TURNS_PER_TICK * 5))) {
        int digestion = SPEED_TO_ENERGY(creature_ptr->pspeed);
        if (creature_ptr->regenerate)
            digestion += 20;
        if (creature_ptr->special_defense & (KAMAE_MASK | KATA_MASK))
            digestion += 20;
        if (creature_ptr->cursed & TRC_FAST_DIGEST)
            digestion += 30;

        if (creature_ptr->slow_digest)
            digestion -= 5;

        if (digestion < 1)
            digestion = 1;
        if (digestion > 100)
            digestion = 100;

        (void)set_food(creature_ptr, creature_ptr->food - digestion);
    }

    if ((creature_ptr->food >= PY_FOOD_FAINT))
        return;

    if (!creature_ptr->paralyzed && (randint0(100) < 10)) {
        msg_print(_("あまりにも空腹で気絶してしまった。", "You faint from the lack of food."));
        disturb(creature_ptr, TRUE, TRUE);
        (void)set_paralyzed(creature_ptr, creature_ptr->paralyzed + 1 + randint0(5));
    }

    if (creature_ptr->food < PY_FOOD_STARVE) {
        HIT_POINT dam = (PY_FOOD_STARVE - creature_ptr->food) / 10;
        if (!is_invuln(creature_ptr))
            take_hit(creature_ptr, DAMAGE_LOSELIFE, dam, _("空腹", "starvation"), -1);
    }
}

/*!
 * @brief 空腹状態をセットする / Set "food", notice observable changes
 * @param v 継続時間
 * @return ステータスに影響を及ぼす変化があった場合TRUEを返す
 * @details
 * Set "", notice observable changes\n
 *\n
 * The "food" variable can get as large as 20000, allowing the
 * addition of the most "filling" item, Elvish Waybread, which adds
 * 7500 food units, without overflowing the 32767 maximum limit.\n
 *\n
 * Perhaps we should disturb the player with various messages,
 * especially messages about hunger status changes.  \n
 *\n
 * Digestion of food is handled in "dungeon.c", in which, normally,
 * the player digests about 20 food units per 100 game turns, more
 * when "fast", more when "regenerating", less with "slow digestion",
 * but when the player is "gorged", he digests 100 food units per 10
 * game turns, or a full 1000 food units per 100 game turns.\n
 *\n
 * Note that the player's speed is reduced by 10 units while gorged,
 * so if the player eats a single food ration (5000 food units) when
 * full (15000 food units), he will be gorged for (5000/100)*10 = 500
 * game turns, or 500/(100/5) = 25 player turns (if nothing else is
 * affecting the player speed).\n
 */
bool set_food(player_type *creature_ptr, TIME_EFFECT v)
{
    int old_aux, new_aux;

    bool notice = FALSE;
    v = (v > 20000) ? 20000 : (v < 0) ? 0 : v;
    if (creature_ptr->food < PY_FOOD_FAINT) {
        old_aux = 0;
    } else if (creature_ptr->food < PY_FOOD_WEAK) {
        old_aux = 1;
    } else if (creature_ptr->food < PY_FOOD_ALERT) {
        old_aux = 2;
    } else if (creature_ptr->food < PY_FOOD_FULL) {
        old_aux = 3;
    } else if (creature_ptr->food < PY_FOOD_MAX) {
        old_aux = 4;
    } else {
        old_aux = 5;
    }

    if (v < PY_FOOD_FAINT) {
        new_aux = 0;
    } else if (v < PY_FOOD_WEAK) {
        new_aux = 1;
    } else if (v < PY_FOOD_ALERT) {
        new_aux = 2;
    } else if (v < PY_FOOD_FULL) {
        new_aux = 3;
    } else if (v < PY_FOOD_MAX) {
        new_aux = 4;
    } else {
        new_aux = 5;
    }

    if (old_aux < 1 && new_aux > 0)
        chg_virtue(creature_ptr, V_PATIENCE, 2);
    else if (old_aux < 3 && (old_aux != new_aux))
        chg_virtue(creature_ptr, V_PATIENCE, 1);
    if (old_aux == 2)
        chg_virtue(creature_ptr, V_TEMPERANCE, 1);
    if (old_aux == 0)
        chg_virtue(creature_ptr, V_TEMPERANCE, -1);

    if (new_aux > old_aux) {
        switch (new_aux) {
        case 1:
            msg_print(_("まだ空腹で倒れそうだ。", "You are still weak."));
            break;
        case 2:
            msg_print(_("まだ空腹だ。", "You are still hungry."));
            break;
        case 3:
            msg_print(_("空腹感がおさまった。", "You are no longer hungry."));
            break;
        case 4:
            msg_print(_("満腹だ！", "You are full!"));
            break;

        case 5:
            msg_print(_("食べ過ぎだ！", "You have gorged yourself!"));
            chg_virtue(creature_ptr, V_HARMONY, -1);
            chg_virtue(creature_ptr, V_PATIENCE, -1);
            chg_virtue(creature_ptr, V_TEMPERANCE, -2);
            break;
        }

        notice = TRUE;
    } else if (new_aux < old_aux) {
        switch (new_aux) {
        case 0:
            msg_print(_("あまりにも空腹で気を失ってしまった！", "You are getting faint from hunger!"));
            break;
        case 1:
            msg_print(_("お腹が空いて倒れそうだ。", "You are getting weak from hunger!"));
            break;
        case 2:
            msg_print(_("お腹が空いてきた。", "You are getting hungry."));
            break;
        case 3:
            msg_print(_("満腹感がなくなった。", "You are no longer full."));
            break;
        case 4:
            msg_print(_("やっとお腹がきつくなくなった。", "You are no longer gorged."));
            break;
        }

        if (creature_ptr->wild_mode && (new_aux < 2)) {
            change_wild_mode(creature_ptr, FALSE);
        }

        notice = TRUE;
    }

    creature_ptr->food = v;
    if (!notice)
        return FALSE;

    if (disturb_state)
        disturb(creature_ptr, FALSE, FALSE);
    creature_ptr->update |= (PU_BONUS);
    creature_ptr->redraw |= (PR_HUNGER);
    handle_stuff(creature_ptr);

    return TRUE;
}
