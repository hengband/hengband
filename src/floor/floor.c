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
 * @brief 所定の位置に上り階段か下り階段を配置する / Place an up/down staircase at given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 配置を試みたいマスのY座標
 * @param x 配置を試みたいマスのX座標
 * @return なし
 */
void place_random_stairs(player_type *player_ptr, POSITION y, POSITION x)
{
    bool up_stairs = TRUE;
    bool down_stairs = TRUE;
    grid_type *g_ptr;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    g_ptr = &floor_ptr->grid_array[y][x];
    if (!is_floor_grid(g_ptr) || g_ptr->o_idx)
        return;

    if (!floor_ptr->dun_level)
        up_stairs = FALSE;
    if (ironman_downward)
        up_stairs = FALSE;
    if (floor_ptr->dun_level >= d_info[player_ptr->dungeon_idx].maxdepth)
        down_stairs = FALSE;
    if (quest_number(player_ptr, floor_ptr->dun_level) && (floor_ptr->dun_level > 1))
        down_stairs = FALSE;

    if (down_stairs && up_stairs) {
        if (randint0(100) < 50)
            up_stairs = FALSE;
        else
            down_stairs = FALSE;
    }

    if (up_stairs)
        set_cave_feat(floor_ptr, y, x, feat_up_stair);
    else if (down_stairs)
        set_cave_feat(floor_ptr, y, x, feat_down_stair);
}

/*!
 * @briefプレイヤーの攻撃射程(マス) / Maximum range (spells, etc)
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 射程
 */
int get_max_range(player_type *creature_ptr) { return creature_ptr->phase_out ? 36 : 18; }

/*
 * Determine if a bolt spell cast from (y1,x1) to (y2,x2) will arrive
 * at the final destination, assuming no monster gets in the way.
 *
 * This is slightly (but significantly) different from "los(y1,x1,y2,x2)".
 */
bool projectable(player_type *player_ptr, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    u16b grid_g[512];
    int grid_n = projection_path(player_ptr, grid_g, (project_length ? project_length : get_max_range(player_ptr)), y1, x1, y2, x2, 0);
    if (!grid_n)
        return TRUE;

    POSITION y = GRID_Y(grid_g[grid_n - 1]);
    POSITION x = GRID_X(grid_g[grid_n - 1]);
    if ((y != y2) || (x != x2))
        return FALSE;

    return TRUE;
}

/*!
 * @brief 指定された座標が地震や階段生成の対象となるマスかを返す。 / Determine if a given location may be "destroyed"
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y y座標
 * @param x x座標
 * @return 各種の変更が可能ならTRUEを返す。
 * @details
 * 条件は永久地形でなく、なおかつ該当のマスにアーティファクトが存在しないか、である。英語の旧コメントに反して＊破壊＊の抑止判定には現在使われていない。
 */
bool cave_valid_bold(floor_type *floor_ptr, POSITION y, POSITION x)
{
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    if (cave_have_flag_grid(g_ptr, FF_PERMANENT))
        return FALSE;

    OBJECT_IDX next_o_idx = 0;
    for (OBJECT_IDX this_o_idx = g_ptr->o_idx; this_o_idx; this_o_idx = next_o_idx) {
        object_type *o_ptr;
        o_ptr = &floor_ptr->o_list[this_o_idx];
        next_o_idx = o_ptr->next_o_idx;
        if (object_is_artifact(o_ptr))
            return FALSE;
    }

    return TRUE;
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
 * @brief 指定のマスが床系地形であるかを返す / Function that sees if a square is a floor.  (Includes range checking.)
 * @param x チェックするマスのX座標
 * @param y チェックするマスのY座標
 * @return 床系地形ならばTRUE
 */
bool get_is_floor(floor_type *floor_ptr, POSITION x, POSITION y)
{
    if (!in_bounds(floor_ptr, y, x)) {
        return FALSE;
    }

    if (is_floor_bold(floor_ptr, y, x))
        return TRUE;

    return FALSE;
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

/*!
 * @brief グローバルオブジェクト配列に対し指定範囲のオブジェクトを整理してIDの若い順に寄せる /
 * Move an object from index i1 to index i2 in the object list
 * @param i1 整理したい配列の始点
 * @param i2 整理したい配列の終点
 * @return なし
 */
static void compact_objects_aux(floor_type *floor_ptr, OBJECT_IDX i1, OBJECT_IDX i2)
{
    if (i1 == i2)
        return;

    object_type *o_ptr;
    for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++) {
        o_ptr = &floor_ptr->o_list[i];
        if (!o_ptr->k_idx)
            continue;

        if (o_ptr->next_o_idx == i1) {
            o_ptr->next_o_idx = i2;
        }
    }

    o_ptr = &floor_ptr->o_list[i1];

    if (object_is_held_monster(o_ptr)) {
        monster_type *m_ptr;
        m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
        if (m_ptr->hold_o_idx == i1) {
            m_ptr->hold_o_idx = i2;
        }
    } else {
        POSITION y = o_ptr->iy;
        POSITION x = o_ptr->ix;
        grid_type *g_ptr;
        g_ptr = &floor_ptr->grid_array[y][x];

        if (g_ptr->o_idx == i1) {
            g_ptr->o_idx = i2;
        }
    }

    floor_ptr->o_list[i2] = floor_ptr->o_list[i1];
    object_wipe(o_ptr);
}

/*!
 * @brief グローバルオブジェクト配列から優先度の低いものを削除し、データを圧縮する。 /
 * Compact and Reorder the object list.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param size 最低でも減らしたいオブジェクト数の水準
 * @return なし
 * @details
 * （危険なので使用には注意すること）
 * This function can be very dangerous, use with caution!\n
 *\n
 * When actually "compacting" objects, we base the saving throw on a\n
 * combination of object level, distance from player, and current\n
 * "desperation".\n
 *\n
 * After "compacting" (if needed), we "reorder" the objects into a more\n
 * compact order, and we reset the allocation info, and the "live" array.\n
 */
void compact_objects(player_type *player_ptr, int size)
{
    object_type *o_ptr;
    if (size) {
        msg_print(_("アイテム情報を圧縮しています...", "Compacting objects..."));
        player_ptr->redraw |= (PR_MAP);
        player_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
    }

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int num = 0, cnt = 1; num < size; cnt++) {
        int cur_lev = 5 * cnt;
        int cur_dis = 5 * (20 - cnt);
        for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++) {
            o_ptr = &floor_ptr->o_list[i];

            if (!object_is_valid(o_ptr))
                continue;
            if (k_info[o_ptr->k_idx].level > cur_lev)
                continue;

            POSITION y, x;
            if (object_is_held_monster(o_ptr)) {
                monster_type *m_ptr;
                m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
                y = m_ptr->fy;
                x = m_ptr->fx;

                if (randint0(100) < 90)
                    continue;
            } else {
                y = o_ptr->iy;
                x = o_ptr->ix;
            }

            if ((cur_dis > 0) && (distance(player_ptr->y, player_ptr->x, y, x) < cur_dis))
                continue;

            int chance = 90;
            if ((object_is_fixed_artifact(o_ptr) || o_ptr->art_name) && (cnt < 1000))
                chance = 100;

            if (randint0(100) < chance)
                continue;

            delete_object_idx(player_ptr, i);
            num++;
        }
    }

    for (OBJECT_IDX i = floor_ptr->o_max - 1; i >= 1; i--) {
        o_ptr = &floor_ptr->o_list[i];
        if (o_ptr->k_idx)
            continue;

        compact_objects_aux(floor_ptr, floor_ptr->o_max - 1, i);
        floor_ptr->o_max--;
    }
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
