#include "system/angband.h"
#include "view/display-birth.h"
#include "player/player-personality.h"
#include "term/gameterm.h"
#include "birth/birth-wizard.h"
#include "birth/birth-stat.h"

/*!
 * @brief オートロール中のステータスを表示する / Display stat values, subset of "put_stats()"
 * @details See 'display_player(p_ptr, )' for screen layout constraints.
 * @return なし
 */
void birth_put_stats(player_type *creature_ptr)
{
    if (!autoroller)
        return;

    const int col = 42;
    for (int i = 0; i < A_MAX; i++) {
        int j = rp_ptr->r_adj[i] + cp_ptr->c_adj[i] + ap_ptr->a_adj[i];
        int m = adjust_stat(creature_ptr->stat_max[i], j);
        char buf[80];
        cnv_stat(m, buf);
        c_put_str(TERM_L_GREEN, buf, 3 + i, col + 24);
        if (stat_match[i]) {
            int p = stat_match[i] > 1000000L ? stat_match[i] / (auto_round / 1000L) : 1000L * stat_match[i] / auto_round;
            TERM_COLOR attr = (p < 100) ? TERM_YELLOW : TERM_L_GREEN;
            sprintf(buf, "%3d.%d%%", p / 10, p % 10);
            c_put_str(attr, buf, 3 + i, col + 13);
        } else {
            c_put_str(TERM_RED, _("(なし)", "(NONE)"), 3 + i, col + 13);
        }
    }
}
