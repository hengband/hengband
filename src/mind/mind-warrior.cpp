#include "mind/mind-warrior.h"
#include "cmd-action/cmd-attack.h"
#include "floor/geometry.h"
#include "spell-kind/spells-teleport.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "target/target-getter.h"
#include "view/display-messages.h"

/*!
 * 戦士と盗賊における、ヒット＆アウェイのレイシャルパワー/突然変異
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return コマンドの入力先にモンスターがいたらTRUE
 */
bool hit_and_away(player_type *caster_ptr)
{
    DIRECTION dir;
    if (!get_direction(caster_ptr, &dir, false, false))
        return false;
    POSITION y = caster_ptr->y + ddy[dir];
    POSITION x = caster_ptr->x + ddx[dir];
    if (caster_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        do_cmd_attack(caster_ptr, y, x, HISSATSU_NONE);
        if (randint0(caster_ptr->skill_dis) < 7)
            msg_print(_("うまく逃げられなかった。", "You failed to run away."));
        else
            teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
        return true;
    }

    msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
    msg_print(nullptr);
    return false;
}

/*!
 * 剣の舞い
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 常にTRUE
 */
bool sword_dancing(player_type *creature_ptr)
{
    DIRECTION dir;
    POSITION y = 0, x = 0;
    grid_type *g_ptr;
    for (int i = 0; i < 6; i++) {
        dir = randint0(8);
        y = creature_ptr->y + ddy_ddd[dir];
        x = creature_ptr->x + ddx_ddd[dir];
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[y][x];

        /* Hack -- attack monsters */
        if (g_ptr->m_idx)
            do_cmd_attack(creature_ptr, y, x, HISSATSU_NONE);
        else {
            msg_print(_("攻撃が空をきった。", "You attack the empty air."));
        }
    }

    return true;
}
