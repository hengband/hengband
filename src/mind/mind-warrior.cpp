﻿#include "mind/mind-warrior.h"
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
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return コマンドの入力先にモンスターがいたらTRUE
 */
bool hit_and_away(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (!get_direction(player_ptr, &dir, false, false))
        return false;
    POSITION y = player_ptr->y + ddy[dir];
    POSITION x = player_ptr->x + ddx[dir];
    if (player_ptr->current_floor_ptr->grid_array[y][x].m_idx) {
        do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
        if (randint0(player_ptr->skill_dis) < 7)
            msg_print(_("うまく逃げられなかった。", "You failed to run away."));
        else
            teleport_player(player_ptr, 30, TELEPORT_SPONTANEOUS);
        return true;
    }

    msg_print(_("その方向にはモンスターはいません。", "You don't see any monster in this direction"));
    msg_print(nullptr);
    return false;
}

/*!
 * 剣の舞い
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 常にTRUE
 */
bool sword_dancing(PlayerType *player_ptr)
{
    DIRECTION dir;
    POSITION y = 0, x = 0;
    grid_type *g_ptr;
    for (int i = 0; i < 6; i++) {
        dir = randint0(8);
        y = player_ptr->y + ddy_ddd[dir];
        x = player_ptr->x + ddx_ddd[dir];
        g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

        /* Hack -- attack monsters */
        if (g_ptr->m_idx)
            do_cmd_attack(player_ptr, y, x, HISSATSU_NONE);
        else {
            msg_print(_("攻撃が空をきった。", "You attack the empty air."));
        }
    }

    return true;
}
