#include "system/angband.h"
#include "cmd-action/cmd-pet.h"
#include "effect/effect-characteristics.h"
#include "mind/racial-mirror-master.h"
#include "spell/process-effect.h"
#include "spell/spells-type.h"
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

/*!
 * @brief 全鏡の消去 / Remove all mirrors in this floor
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param explode 爆発処理を伴うならばTRUE
 * @return なし
 */
void remove_all_mirrors(player_type *caster_ptr, bool explode)
{
    for (POSITION x = 0; x < caster_ptr->current_floor_ptr->width; x++) {
        for (POSITION y = 0; y < caster_ptr->current_floor_ptr->height; y++) {
            if (!is_mirror_grid(&caster_ptr->current_floor_ptr->grid_array[y][x]))
                continue;

            remove_mirror(caster_ptr, y, x);
            if (!explode)
                continue;

            project(caster_ptr, 0, 2, y, x, caster_ptr->lev / 2 + 5, GF_SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_JUMP | PROJECT_NO_HANGEKI), -1);
        }
    }
}
