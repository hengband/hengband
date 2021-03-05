﻿#include "view/display-birth.h"
#include "birth/auto-roller.h"
#include "birth/birth-stat.h"
#include "game-option/birth-options.h"
#include "player/player-class.h"
#include "player/player-personality.h"
#include "player/player-race.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"

/*!
 * @brief オートロール中のステータスを表示する / Display stat values, subset of "put_stats()"
 * @details See 'display_player(p_ptr, )' for screen layout constraints.
 * @return なし
 */
void birth_put_stats(player_type *creature_ptr)
{
    if (!autoroller)
        return;

    const int col = 22;
    for (int i = 0; i < A_MAX; i++) {
        int j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];
        int m = adjust_stat(creature_ptr->stat_max[i], j);
        char buf[80];
        cnv_stat(m, buf);
        c_put_str(TERM_L_GREEN, buf, 3 + i, col + 24);
    }
}
