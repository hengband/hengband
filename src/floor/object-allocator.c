#include "floor/object-allocator.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-allocation-types.h"
#include "floor/floor-generator-util.h"
#include "floor/dungeon-tunnel-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief 上下左右の外壁数をカウントする / Count the number of walls adjacent to the given grid.
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @return 隣接する外壁の数
 * @note Assumes "in_bounds()"
 * @details We count only granite walls and permanent walls.
 */
static int next_to_walls(floor_type *floor_ptr, POSITION y, POSITION x)
{
    int k = 0;
    if (in_bounds(floor_ptr, y + 1, x) && is_extra_bold(floor_ptr, y + 1, x))
        k++;

    if (in_bounds(floor_ptr, y - 1, x) && is_extra_bold(floor_ptr, y - 1, x))
        k++;

    if (in_bounds(floor_ptr, y, x + 1) && is_extra_bold(floor_ptr, y, x + 1))
        k++;

    if (in_bounds(floor_ptr, y, x - 1) && is_extra_bold(floor_ptr, y, x - 1))
        k++;

    return k;
}

/*!
 * @brief alloc_stairs()の補助として指定の位置に階段を生成できるかの判定を行う / Helper function for alloc_stairs(). Is this a good location for stairs?
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @param walls 最低減隣接させたい外壁の数
 * @return 階段を生成して問題がないならばTRUEを返す。
 */
static bool alloc_stairs_aux(player_type *player_ptr, POSITION y, POSITION x, int walls)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    if (!is_floor_grid(g_ptr) || pattern_tile(floor_ptr, y, x) || (g_ptr->o_idx != 0) || (g_ptr->m_idx != 0) || next_to_walls(floor_ptr, y, x) < walls)
        return FALSE;

    return TRUE;
}

/*!
 * @brief 外壁に隣接させて階段を生成する / Places some staircases near walls
 * @param owner_ptr プレーヤーへの参照ポインタ
 * @param feat 配置したい地形ID
 * @param num 配置したい階段の数
 * @param walls 最低減隣接させたい外壁の数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
bool alloc_stairs(player_type *owner_ptr, FEAT_IDX feat, int num, int walls)
{
    int shaft_num = 0;
    feature_type *f_ptr = &f_info[feat];
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    if (has_flag(f_ptr->flags, FF_LESS)) {
        if (ironman_downward || !floor_ptr->dun_level)
            return TRUE;

        if (floor_ptr->dun_level > d_info[floor_ptr->dungeon_idx].mindepth)
            shaft_num = (randint1(num + 1)) / 2;
    } else if (has_flag(f_ptr->flags, FF_MORE)) {
        QUEST_IDX q_idx = quest_number(owner_ptr, floor_ptr->dun_level);
        if (floor_ptr->dun_level > 1 && q_idx) {
            monster_race *r_ptr = &r_info[quest[q_idx].r_idx];
            if (!(r_ptr->flags1 & RF1_UNIQUE) || 0 < r_ptr->max_num)
                return TRUE;
        }

        if (floor_ptr->dun_level >= d_info[floor_ptr->dungeon_idx].maxdepth)
            return TRUE;

        if ((floor_ptr->dun_level < d_info[floor_ptr->dungeon_idx].maxdepth - 1) && !quest_number(owner_ptr, floor_ptr->dun_level + 1))
            shaft_num = (randint1(num) + 1) / 2;
    } else
        return FALSE;

    for (int i = 0; i < num; i++) {
        while (TRUE) {
            grid_type *g_ptr;
            int candidates = 0;
            const POSITION max_x = floor_ptr->width - 1;
            for (POSITION y = 1; y < floor_ptr->height - 1; y++)
                for (POSITION x = 1; x < max_x; x++)
                    if (alloc_stairs_aux(owner_ptr, y, x, walls))
                        candidates++;

            if (!candidates) {
                if (walls <= 0)
                    return FALSE;

                walls--;
                continue;
            }

            int pick = randint1(candidates);
            POSITION y;
            POSITION x = max_x;
            for (y = 1; y < floor_ptr->height - 1; y++) {
                for (x = 1; x < floor_ptr->width - 1; x++) {
                    if (alloc_stairs_aux(owner_ptr, y, x, walls)) {
                        pick--;
                        if (pick == 0)
                            break;
                    }
                }

                if (pick == 0)
                    break;
            }

            g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->mimic = 0;
            g_ptr->feat = (i < shaft_num) ? feat_state(owner_ptr, feat, FF_SHAFT) : feat;
            g_ptr->info &= ~(CAVE_FLOOR);
            break;
        }
    }

    return TRUE;
}

/*!
 * @brief フロア上のランダム位置に各種オブジェクトを配置する / Allocates some objects (using "place" and "type")
 * @param set 配置したい地形の種類
 * @param typ 配置したいオブジェクトの種類
 * @param num 配置したい数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
void alloc_object(player_type *owner_ptr, dap_type set, EFFECT_ID typ, int num)
{
    POSITION y = 0;
    POSITION x = 0;
    int dummy = 0;
    grid_type *g_ptr;
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    num = num * floor_ptr->height * floor_ptr->width / (MAX_HGT * MAX_WID) + 1;
    for (int k = 0; k < num; k++) {
        while (dummy < SAFE_MAX_ATTEMPTS) {
            dummy++;
            y = randint0(floor_ptr->height);
            x = randint0(floor_ptr->width);
            g_ptr = &floor_ptr->grid_array[y][x];
            if (!is_floor_grid(g_ptr) || g_ptr->o_idx || g_ptr->m_idx)
                continue;

            if (player_bold(owner_ptr, y, x))
                continue;

            bool room = (floor_ptr->grid_array[y][x].info & CAVE_ROOM) ? TRUE : FALSE;
            if (((set == ALLOC_SET_CORR) && room) || ((set == ALLOC_SET_ROOM) && !room))
                continue;

            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS) {
            msg_print_wizard(owner_ptr, CHEAT_DUNGEON, _("アイテムの配置に失敗しました。", "Failed to place object."));
            return;
        }

        switch (typ) {
        case ALLOC_TYP_RUBBLE:
            place_rubble(floor_ptr, y, x);
            floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_TRAP:
            place_trap(owner_ptr, y, x);
            floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_GOLD:
            place_gold(owner_ptr, y, x);
            break;
        case ALLOC_TYP_OBJECT:
            place_object(owner_ptr, y, x, 0L);
            break;
        }
    }
}
