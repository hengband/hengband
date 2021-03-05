﻿/*!
 * @brief フロア全体の処理に関するユーティリティ
 * @date 2019/04/24
 * @author deskull
 */
#include "floor/floor-util.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-town.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "object/object-generator.h"
#include "perception/object-perception.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "target/projection-path-calculator.h"
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
            if (!cave_has_flag_grid(g_ptr, FF_MOVE) && !is_closed_door(subject_ptr, g_ptr->feat))
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

/*!
 * @brief 現在のマップ名を返す /
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return マップ名の文字列参照ポインタ
 */
concptr map_name(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest) && (quest[floor_ptr->inside_quest].flags & QUEST_FLAG_PRESET))
        return _("クエスト", "Quest");
    else if (creature_ptr->wild_mode)
        return _("地上", "Surface");
    else if (floor_ptr->inside_arena)
        return _("アリーナ", "Arena");
    else if (creature_ptr->phase_out)
        return _("闘技場", "Monster Arena");
    else if (!floor_ptr->dun_level && creature_ptr->town_num)
        return town_info[creature_ptr->town_num].name;
    else
        return d_name + d_info[creature_ptr->dungeon_idx].name;
}
