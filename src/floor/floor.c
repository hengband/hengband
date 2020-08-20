#include "floor/floor.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "effect/spells-effect-util.h"
#include "floor/cave.h"
#include "floor/floor-generator-util.h"
#include "floor/floor-object.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/map-screen-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-update.h"
#include "object-enchant/special-object-flags.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-generator.h"
#include "object/object-kind.h"
#include "perception/object-perception.h"
#include "player/special-defense-types.h"
#include "room/door-definition.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world-object.h"
#include "world/world.h"

/*
 * The array of floor [MAX_WID][MAX_HGT].
 * Not completely allocated, that would be inefficient
 * Not completely hardcoded, that would overflow memory
 */
floor_type floor_info;

static int scent_when = 0;

/*
 * Characters leave scent trails for perceptive monsters to track.
 *
 * Smell is rather more limited than sound.  Many creatures cannot use
 * it at all, it doesn't extend very far outwards from the character's
 * current position, and monsters can use it to home in the character,
 * but not to run away from him.
 *
 * Smell is valued according to age.  When a character takes his turn,
 * scent is aged by one, and new scent of the current age is laid down.
 * Speedy characters leave more scent, true, but it also ages faster,
 * which makes it harder to hunt them down.
 *
 * Whenever the age count loops, most of the scent trail is erased and
 * the age of the remainder is recalculated.
 */
void update_smell(floor_type *floor_ptr, player_type *subject_ptr)
{
    /* Create a table that controls the spread of scent */
    const int scent_adjust[5][5] = {
        { -1, 0, 0, 0, -1 },
        { 0, 1, 1, 1, 0 },
        { 0, 1, 2, 1, 0 },
        { 0, 1, 1, 1, 0 },
        { -1, 0, 0, 0, -1 },
    };

    if (++scent_when == 254) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            for (POSITION x = 0; x < floor_ptr->width; x++) {
                int w = floor_ptr->grid_array[y][x].when;
                floor_ptr->grid_array[y][x].when = (w > 128) ? (w - 128) : 0;
            }
        }

        scent_when = 126;
    }

    for (POSITION i = 0; i < 5; i++) {
        for (POSITION j = 0; j < 5; j++) {
            grid_type *g_ptr;
            POSITION y = i + subject_ptr->y - 2;
            POSITION x = j + subject_ptr->x - 2;
            if (!in_bounds(floor_ptr, y, x))
                continue;

            g_ptr = &floor_ptr->grid_array[y][x];
            if (!cave_have_flag_grid(g_ptr, FF_MOVE) && !is_closed_door(subject_ptr, g_ptr->feat))
                continue;
            if (!player_has_los_bold(subject_ptr, y, x))
                continue;
            if (scent_adjust[i][j] == -1)
                continue;

            g_ptr->when = scent_when + scent_adjust[i][j];
        }
    }
}

/*
 * Hack -- forget the "flow" information
 */
void forget_flow(floor_type *floor_ptr)
{
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            floor_ptr->grid_array[y][x].dist = 0;
            floor_ptr->grid_array[y][x].cost = 0;
            floor_ptr->grid_array[y][x].when = 0;
        }
    }
}

/*!
 * @brief グローバルオブジェクト配列を初期化する /
 * Delete all the items when player leaves the level
 * @note we do NOT visually reflect these (irrelevant) changes
 * @details
 * Hack -- we clear the "g_ptr->o_idx" field for every grid,
 * and the "m_ptr->next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 * @return なし
 */
void wipe_o_list(floor_type *floor_ptr)
{
    for (int i = 1; i < floor_ptr->o_max; i++) {
        object_type *o_ptr = &floor_ptr->o_list[i];
        if (!object_is_valid(o_ptr))
            continue;

        if (!current_world_ptr->character_dungeon || preserve_mode) {
            if (object_is_fixed_artifact(o_ptr) && !object_is_known(o_ptr)) {
                a_info[o_ptr->name1].cur_num = 0;
            }
        }

        if (object_is_held_monster(o_ptr)) {
            monster_type *m_ptr;
            m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
            m_ptr->hold_o_idx = 0;
            object_wipe(o_ptr);
            continue;
        }

        grid_type *g_ptr;
        POSITION y = o_ptr->iy;
        POSITION x = o_ptr->ix;

        g_ptr = &floor_ptr->grid_array[y][x];
        g_ptr->o_idx = 0;
        object_wipe(o_ptr);
    }

    floor_ptr->o_max = 1;
    floor_ptr->o_cnt = 0;
}

/*!
 * @brief 指定のマスを床地形に変える / Set a square to be floor.  (Includes range checking.)
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param x 地形を変えたいマスのX座標
 * @param y 地形を変えたいマスのY座標
 * @return なし
 */
void set_floor(player_type *player_ptr, POSITION x, POSITION y)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, y, x)) {
        return;
    }

    if (floor_ptr->grid_array[y][x].info & CAVE_ROOM) {
        return;
    }

    if (is_extra_bold(floor_ptr, y, x))
        place_bold(player_ptr, y, x, GB_FLOOR);
}

/*
 * Standard "find me a location" function
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "los()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * Currently the "m" parameter is unused.
 */
void scatter(player_type *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    POSITION nx, ny;
    while (TRUE) {
        ny = rand_spread(y, d);
        nx = rand_spread(x, d);

        if (!in_bounds(floor_ptr, ny, nx))
            continue;
        if ((d > 1) && (distance(y, x, ny, nx) > d))
            continue;
        if (mode & PROJECT_LOS) {
            if (los(player_ptr, y, x, ny, nx))
                break;
            continue;
        }

        if (projectable(player_ptr, y, x, ny, nx))
            break;
    }

    *yp = ny;
    *xp = nx;
}
