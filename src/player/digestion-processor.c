#include "system/angband.h"
#include "player/digestion-processor.h"
#include "world/world.h"
#include "realm/realm-song.h"
#include "player/player-damage.h"
#include "player/player-effects.h"
#include "monster/creature.h"
#include "player/player-move.h"
#include "object/trc-types.h"

/*!
 * @brief 10ゲームターンが進行するごとにプレイヤーの腹を減らす
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void starve_player(player_type* creature_ptr)
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
        if (!IS_INVULN(creature_ptr))
            take_hit(creature_ptr, DAMAGE_LOSELIFE, dam, _("空腹", "starvation"), -1);
    }
}
