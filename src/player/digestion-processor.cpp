#include "player/digestion-processor.h"
#include "avatar/avatar.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/speed-table.h"
#include "core/stuff-handler.h"
#include "floor/wild.h"
#include "game-option/disturbance-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "object-enchant/trc-types.h"
#include "player-base/player-class.h"
#include "player-info/monk-data-type.h"
#include "player-info/samurai-data-type.h"
#include "player/player-damage.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "status/bad-status-setter.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-paralysis.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーの腹を減らす
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void starve_player(PlayerType *player_ptr)
{
    if (player_ptr->phase_out) {
        return;
    }

    if (player_ptr->food >= PY_FOOD_MAX) {
        (void)set_food(player_ptr, player_ptr->food - 100);
    } else if (!(w_ptr->game_turn % (TURNS_PER_TICK * 5))) {
        int digestion = speed_to_energy(player_ptr->pspeed);
        if (player_ptr->regenerate) {
            digestion += 20;
        }
        PlayerClass pc(player_ptr);
        if (!pc.monk_stance_is(MonkStanceType::NONE) || !pc.samurai_stance_is(SamuraiStanceType::NONE)) {
            digestion += 20;
        }
        if (player_ptr->cursed.has(CurseTraitType::FAST_DIGEST)) {
            digestion += 30;
        }

        if (player_ptr->slow_digest) {
            digestion -= 5;
        }

        if (digestion < 1) {
            digestion = 1;
        }
        if (digestion > 100) {
            digestion = 100;
        }

        (void)set_food(player_ptr, player_ptr->food - digestion);
    }

    if ((player_ptr->food >= PY_FOOD_FAINT)) {
        return;
    }

    if (!player_ptr->effects()->paralysis()->is_paralyzed() && (randint0(100) < 10)) {
        msg_print(_("あまりにも空腹で気絶してしまった。", "You faint from the lack of food."));
        disturb(player_ptr, true, true);
        (void)BadStatusSetter(player_ptr).mod_paralysis(1 + randint0(5));
    }

    if (player_ptr->food < PY_FOOD_STARVE) {
        int dam = (PY_FOOD_STARVE - player_ptr->food) / 10;
        if (!is_invuln(player_ptr)) {
            take_hit(player_ptr, DAMAGE_LOSELIFE, dam, _("空腹", "starvation"));
        }
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
bool set_food(PlayerType *player_ptr, TIME_EFFECT v)
{
    int old_aux, new_aux;

    bool notice = false;
    v = (v > 20000) ? 20000 : (v < 0) ? 0
                                      : v;
    if (player_ptr->food < PY_FOOD_FAINT) {
        old_aux = 0;
    } else if (player_ptr->food < PY_FOOD_WEAK) {
        old_aux = 1;
    } else if (player_ptr->food < PY_FOOD_ALERT) {
        old_aux = 2;
    } else if (player_ptr->food < PY_FOOD_FULL) {
        old_aux = 3;
    } else if (player_ptr->food < PY_FOOD_MAX) {
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

    if (old_aux < 1 && new_aux > 0) {
        chg_virtue(player_ptr, V_PATIENCE, 2);
    } else if (old_aux < 3 && (old_aux != new_aux)) {
        chg_virtue(player_ptr, V_PATIENCE, 1);
    }
    if (old_aux == 2) {
        chg_virtue(player_ptr, V_TEMPERANCE, 1);
    }
    if (old_aux == 0) {
        chg_virtue(player_ptr, V_TEMPERANCE, -1);
    }

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
            chg_virtue(player_ptr, V_HARMONY, -1);
            chg_virtue(player_ptr, V_PATIENCE, -1);
            chg_virtue(player_ptr, V_TEMPERANCE, -2);
            break;
        }

        notice = true;
    } else if (new_aux < old_aux) {
        switch (new_aux) {
        case 0:
            sound(SOUND_FAINT);
            msg_print(_("あまりにも空腹で気を失ってしまった！", "You are getting faint from hunger!"));
            break;
        case 1:
            sound(SOUND_WEAK);
            msg_print(_("お腹が空いて倒れそうだ。", "You are getting weak from hunger!"));
            break;
        case 2:
            sound(SOUND_HUNGRY);
            msg_print(_("お腹が空いてきた。", "You are getting hungry."));
            break;
        case 3:
            msg_print(_("満腹感がなくなった。", "You are no longer full."));
            break;
        case 4:
            msg_print(_("やっとお腹がきつくなくなった。", "You are no longer gorged."));
            break;
        }

        if (player_ptr->wild_mode && (new_aux < 2)) {
            change_wild_mode(player_ptr, false);
        }

        notice = true;
    }

    player_ptr->food = v;
    if (!notice) {
        return false;
    }

    if (disturb_state) {
        disturb(player_ptr, false, false);
    }
    player_ptr->update |= (PU_BONUS);
    player_ptr->redraw |= (PR_HUNGER);
    handle_stuff(player_ptr);

    return true;
}
