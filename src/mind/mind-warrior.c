#include "mind/mind-warrior.h"
#include "cmd-action/cmd-attack.h"
#include "floor/floor.h"
#include "io/targeting.h"
#include "spell-kind/spells-teleport.h"

/*!
 * 戦士と盗賊における、ヒット＆アウェイのレイシャルパワー/突然変異
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @return コマンドの入力先にモンスターがいたらTRUE
 */
bool hit_and_away(player_type *caster_ptr)
{
    DIRECTION dir;
    if (!get_direction(caster_ptr, &dir, FALSE, FALSE))
        return FALSE;
    POSITION y = caster_ptr->y + ddy[dir];
    POSITION x = caster_ptr->x + ddx[dir];
    if (caster_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        do_cmd_attack(caster_ptr, y, x, 0);
        if (randint0(caster_ptr->skill_dis) < 7)
            msg_print(_("うまく逃げられなかった。", "You failed to run away."));
        else
            teleport_player(caster_ptr, 30, TELEPORT_SPONTANEOUS);
        return TRUE;
    }

    msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
    msg_print(NULL);
    return FALSE;
}
