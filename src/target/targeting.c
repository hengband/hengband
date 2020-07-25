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

#include "target/targeting.h"
#include "action/travel-execution.h"
#include "cmd-action/cmd-pet.h"
#include "cmd-building/cmd-building.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/spells-effect-util.h"
#include "flavor/flavor-describer.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-object.h"
#include "floor/floor-town.h"
#include "floor/floor.h"
#include "floor/object-scanner.h"
#include "game-option/cheat-options.h"
#include "game-option/disturbance-options.h"
#include "game-option/game-play-options.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "game-option/map-screen-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "io/command-repeater.h"
#include "io/cursor.h"
#include "io/input-key-acceptor.h"
#include "io/input-key-requester.h"
#include "io/screen-util.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race-hook.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/object-curse.h"
#include "object/object-kind-hook.h"
#include "object/object-mark-types.h"
#include "player/player-race-types.h"
#include "player/player-status.h"
#include "spell/spells-summon.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "target/target-describer.h"
#include "target/target-preparation.h"
#include "target/target-setter.h"
#include "target/target-types.h" // todo 相互依存.
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/bit-flags-calculator.h"
#include "util/int-char-converter.h"
#include "util/sort.h"
#include "view/display-lore.h"
#include "view/display-messages.h"
#include "view/display-monster-status.h"
#include "window/main-window-util.h"
#include "world/world.h"

/* Targetting variables */
MONSTER_IDX target_who;
POSITION target_col;
POSITION target_row;

/*!
 * @brief マップ描画のフォーカスを当てるべき座標を更新する
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details
 * Given an row (y) and col (x), this routine detects when a move
 * off the screen has occurred and figures new borders. -RAK-
 * "Update" forces a "full update" to take place.
 * The map is reprinted if necessary, and "TRUE" is returned.
 * @return 実際に再描画が必要だった場合TRUEを返す
 */
void verify_panel(player_type *creature_ptr)
{
    POSITION y = creature_ptr->y;
    POSITION x = creature_ptr->x;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    int max_prow_min = creature_ptr->current_floor_ptr->height - hgt;
    int max_pcol_min = creature_ptr->current_floor_ptr->width - wid;
    if (max_prow_min < 0)
        max_prow_min = 0;
    if (max_pcol_min < 0)
        max_pcol_min = 0;

    int prow_min;
    int pcol_min;
    if (center_player && (center_running || !creature_ptr->running)) {
        prow_min = y - hgt / 2;
        if (prow_min < 0)
            prow_min = 0;
        else if (prow_min > max_prow_min)
            prow_min = max_prow_min;

        pcol_min = x - wid / 2;
        if (pcol_min < 0)
            pcol_min = 0;
        else if (pcol_min > max_pcol_min)
            pcol_min = max_pcol_min;
    } else {
        prow_min = panel_row_min;
        pcol_min = panel_col_min;
        if (y > panel_row_max - 2)
            while (y > prow_min + hgt - 1 - 2)
                prow_min += (hgt / 2);

        if (y < panel_row_min + 2)
            while (y < prow_min + 2)
                prow_min -= (hgt / 2);

        if (prow_min > max_prow_min)
            prow_min = max_prow_min;

        if (prow_min < 0)
            prow_min = 0;

        if (x > panel_col_max - 4)
            while (x > pcol_min + wid - 1 - 4)
                pcol_min += (wid / 2);

        if (x < panel_col_min + 4)
            while (x < pcol_min + 4)
                pcol_min -= (wid / 2);

        if (pcol_min > max_pcol_min)
            pcol_min = max_pcol_min;

        if (pcol_min < 0)
            pcol_min = 0;
    }

    if ((prow_min == panel_row_min) && (pcol_min == panel_col_min))
        return;

    panel_row_min = prow_min;
    panel_col_min = pcol_min;
    if (disturb_panel && !center_player)
        disturb(creature_ptr, FALSE, FALSE);

    panel_bounds_center();
    creature_ptr->update |= (PU_MONSTERS);
    creature_ptr->redraw |= (PR_MAP);
    creature_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
}

/*
 * Update (if necessary) and verify (if possible) the target.
 * We return TRUE if the target is "okay" and FALSE otherwise.
 */
bool target_okay(player_type *creature_ptr)
{
    if (target_who < 0)
        return TRUE;

    if (target_who <= 0)
        return FALSE;

    if (!target_able(creature_ptr, target_who))
        return FALSE;

    monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[target_who];
    target_row = m_ptr->fy;
    target_col = m_ptr->fx;
    return TRUE;
}
