#include "racial/racial-vampire.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/geometry.h"
#include "grid/grid.h"
#include "hpmp/hp-mp-processor.h"
#include "player/digestion-processor.h"
#include "player/player-status.h"
#include "spell-kind/spells-specific-bolt.h"
#include "system/floor-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

bool vampirism(player_type *caster_ptr)
{
    if (d_info[caster_ptr->dungeon_idx].flags.has(DF::NO_MELEE)) {
        msg_print(_("なぜか攻撃することができない。", "Something prevents you from attacking."));
        return false;
    }

    DIRECTION dir;
    if (!get_direction(caster_ptr, &dir, false, false))
        return false;

    POSITION y = caster_ptr->y + ddy[dir];
    POSITION x = caster_ptr->x + ddx[dir];
    grid_type *g_ptr;
    g_ptr = &caster_ptr->current_floor_ptr->grid_array[y][x];
    stop_mouth(caster_ptr);
    if (!(g_ptr->m_idx)) {
        msg_print(_("何もない場所に噛みついた！", "You bite into thin air!"));
        return false;
    }

    msg_print(_("あなたはニヤリとして牙をむいた...", "You grin and bare your fangs..."));

    int dummy = caster_ptr->lev * 2;
    if (!hypodynamic_bolt(caster_ptr, dir, dummy)) {
        msg_print(_("げぇ！ひどい味だ。", "Yechh. That tastes foul."));
        return true;
    }

    if (caster_ptr->food < PY_FOOD_FULL)
        (void)hp_player(caster_ptr, dummy);
    else
        msg_print(_("あなたは空腹ではありません。", "You were not hungry."));

    /* Gain nutritional sustenance: 150/hp drained */
    /* A Food ration gives 5000 food points (by contrast) */
    /* Don't ever get more than "Full" this way */
    /* But if we ARE Gorged,  it won't cure us */
    dummy = caster_ptr->food + MIN(5000, 100 * dummy);
    if (caster_ptr->food < PY_FOOD_MAX) /* Not gorged already */
        (void)set_food(caster_ptr, dummy >= PY_FOOD_MAX ? PY_FOOD_MAX - 1 : dummy);

    return true;
}
