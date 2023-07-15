/*!
 * @brief 雑多なその他の処理2 / effects of various "objects"
 * @date 2014/02/06
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research, and\n
 * not for profit purposes provided that this copyright and statement are\n
 * included in all such copies.\n
 * 2014 Deskull rearranged comment for Doxygen.
 */

#include "target/target-checker.h"
#include "core/disturbance.h"
#include "core/window-redrawer.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-preparation.h"
#include "target/target-types.h"
#include "window/main-window-util.h"

/* Targetting variables */
MONSTER_IDX target_who;
POSITION target_col;
POSITION target_row;

/*!
 * @brief マップ描画のフォーカスを当てるべき座標を更新する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 * "Update" forces a "full update" to take place.
 * The map is reprinted if necessary, and "TRUE" is returned.
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
void verify_panel(PlayerType *player_ptr)
{
    POSITION y = player_ptr->y;
    POSITION x = player_ptr->x;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    int max_prow_min = player_ptr->current_floor_ptr->height - hgt;
    int max_pcol_min = player_ptr->current_floor_ptr->width - wid;
    if (max_prow_min < 0) {
        max_prow_min = 0;
    }
    if (max_pcol_min < 0) {
        max_pcol_min = 0;
    }

    int prow_min;
    int pcol_min;
    if (center_player && (center_running || !player_ptr->running)) {
        prow_min = y - hgt / 2;
        if (prow_min < 0) {
            prow_min = 0;
        } else if (prow_min > max_prow_min) {
            prow_min = max_prow_min;
        }

        pcol_min = x - wid / 2;
        if (pcol_min < 0) {
            pcol_min = 0;
        } else if (pcol_min > max_pcol_min) {
            pcol_min = max_pcol_min;
        }
    } else {
        prow_min = panel_row_min;
        pcol_min = panel_col_min;
        if (y > panel_row_max - 2) {
            while (y > prow_min + hgt - 1 - 2) {
                prow_min += (hgt / 2);
            }
        }

        if (y < panel_row_min + 2) {
            while (y < prow_min + 2) {
                prow_min -= (hgt / 2);
            }
        }

        if (prow_min > max_prow_min) {
            prow_min = max_prow_min;
        }

        if (prow_min < 0) {
            prow_min = 0;
        }

        if (x > panel_col_max - 4) {
            while (x > pcol_min + wid - 1 - 4) {
                pcol_min += (wid / 2);
            }
        }

        if (x < panel_col_min + 4) {
            while (x < pcol_min + 4) {
                pcol_min -= (wid / 2);
            }
        }

        if (pcol_min > max_pcol_min) {
            pcol_min = max_pcol_min;
        }

        if (pcol_min < 0) {
            pcol_min = 0;
        }
    }

    if ((prow_min == panel_row_min) && (pcol_min == panel_col_min)) {
        return;
    }

    panel_row_min = prow_min;
    panel_col_min = pcol_min;
    if (disturb_panel && !center_player) {
        disturb(player_ptr, false, false);
    }

    panel_bounds_center();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags);
}

/*
 * Update (if necessary) and verify (if possible) the target.
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(PlayerType *player_ptr)
{
    if (target_who < 0) {
        return true;
    }

    if (target_who <= 0) {
        return false;
    }

    if (!target_able(player_ptr, target_who)) {
        return false;
    }

    auto *m_ptr = &player_ptr->current_floor_ptr->m_list[target_who];
    target_row = m_ptr->fy;
    target_col = m_ptr->fx;
    return true;
}
