#include "floor/object-allocator.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/dungeon-tunnel-util.h"
#include "floor/floor-allocation-types.h"
#include "floor/floor-generator-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
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
    if (in_bounds(floor_ptr, y + 1, x) && floor_ptr->grid_array[y + 1][x].is_extra())
        k++;

    if (in_bounds(floor_ptr, y - 1, x) && floor_ptr->grid_array[y - 1][x].is_extra())
        k++;

    if (in_bounds(floor_ptr, y, x + 1) && floor_ptr->grid_array[y][x + 1].is_extra())
        k++;

    if (in_bounds(floor_ptr, y, x - 1) && floor_ptr->grid_array[y][x - 1].is_extra())
        k++;

    return k;
}

/*!
 * @brief alloc_stairs()の補助として指定の位置に階段を生成できるかの判定を行う / Helper function for alloc_stairs(). Is this a good location for stairs?
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @param walls 最低減隣接させたい外壁の数
 * @return 階段を生成して問題がないならばTRUEを返す。
 */
static bool alloc_stairs_aux(PlayerType *player_ptr, POSITION y, POSITION x, int walls)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    if (!g_ptr->is_floor() || pattern_tile(floor_ptr, y, x) || !g_ptr->o_idx_list.empty() || (g_ptr->m_idx != 0) || next_to_walls(floor_ptr, y, x) < walls)
        return false;

    return true;
}

/*!
 * @brief 外壁に隣接させて階段を生成する / Places some staircases near walls
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param feat 配置したい地形ID
 * @param num 配置したい階段の数
 * @param walls 最低減隣接させたい外壁の数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
bool alloc_stairs(PlayerType *player_ptr, FEAT_IDX feat, int num, int walls)
{
    int shaft_num = 0;
    feature_type *f_ptr = &f_info[feat];
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (f_ptr->flags.has(FloorFeatureType::LESS)) {
        if (ironman_downward || !floor_ptr->dun_level)
            return true;

        if (floor_ptr->dun_level > d_info[floor_ptr->dungeon_idx].mindepth)
            shaft_num = (randint1(num + 1)) / 2;
    } else if (f_ptr->flags.has(FloorFeatureType::MORE)) {
        QUEST_IDX q_idx = quest_number(player_ptr, floor_ptr->dun_level);
        if (floor_ptr->dun_level > 1 && q_idx) {
            monster_race *r_ptr = &r_info[quest[q_idx].r_idx];
            if (!(r_ptr->flags1 & RF1_UNIQUE) || 0 < r_ptr->max_num)
                return true;
        }

        if (floor_ptr->dun_level >= d_info[floor_ptr->dungeon_idx].maxdepth)
            return true;

        if ((floor_ptr->dun_level < d_info[floor_ptr->dungeon_idx].maxdepth - 1) && !quest_number(player_ptr, floor_ptr->dun_level + 1))
            shaft_num = (randint1(num) + 1) / 2;
    } else
        return false;

    for (int i = 0; i < num; i++) {
        while (true) {
            grid_type *g_ptr;
            int candidates = 0;
            const POSITION max_x = floor_ptr->width - 1;
            for (POSITION y = 1; y < floor_ptr->height - 1; y++)
                for (POSITION x = 1; x < max_x; x++)
                    if (alloc_stairs_aux(player_ptr, y, x, walls))
                        candidates++;

            if (!candidates) {
                if (walls <= 0)
                    return false;

                walls--;
                continue;
            }

            int pick = randint1(candidates);
            POSITION y;
            POSITION x = max_x;
            for (y = 1; y < floor_ptr->height - 1; y++) {
                for (x = 1; x < floor_ptr->width - 1; x++) {
                    if (alloc_stairs_aux(player_ptr, y, x, walls)) {
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
            g_ptr->feat = (i < shaft_num) ? feat_state(player_ptr->current_floor_ptr, feat, FloorFeatureType::SHAFT) : feat;
            g_ptr->info &= ~(CAVE_FLOOR);
            break;
        }
    }

    return true;
}

/*!
 * @brief 指定座標に瓦礫を配置する
 * @param Y 指定Y座標
 * @param X 指定X座標
 */
static void place_rubble(floor_type *floor_ptr, POSITION y, POSITION x)
{
    set_cave_feat(floor_ptr, y, x, feat_rubble);
}

/*!
 * @brief フロア上のランダム位置に各種オブジェクトを配置する / Allocates some objects (using "place" and "type")
 * @param set 配置したい地形の種類
 * @param typ 配置したいオブジェクトの種類
 * @param num 配置したい数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
void alloc_object(PlayerType *player_ptr, dap_type set, EFFECT_ID typ, int num)
{
    POSITION y = 0;
    POSITION x = 0;
    int dummy = 0;
    grid_type *g_ptr;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    num = num * floor_ptr->height * floor_ptr->width / (MAX_HGT * MAX_WID) + 1;
    for (int k = 0; k < num; k++) {
        while (dummy < SAFE_MAX_ATTEMPTS) {
            dummy++;
            y = randint0(floor_ptr->height);
            x = randint0(floor_ptr->width);
            g_ptr = &floor_ptr->grid_array[y][x];
            if (!g_ptr->is_floor() || !g_ptr->o_idx_list.empty() || g_ptr->m_idx)
                continue;

            if (player_bold(player_ptr, y, x))
                continue;

            auto is_room = floor_ptr->grid_array[y][x].is_room();
            if (((set == ALLOC_SET_CORR) && is_room) || ((set == ALLOC_SET_ROOM) && !is_room))
                continue;

            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS) {
            msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("アイテムの配置に失敗しました。", "Failed to place object."));
            return;
        }

        switch (typ) {
        case ALLOC_TYP_RUBBLE:
            place_rubble(floor_ptr, y, x);
            floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_TRAP:
            place_trap(player_ptr, y, x);
            floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_GOLD:
            place_gold(player_ptr, y, x);
            break;
        case ALLOC_TYP_OBJECT:
            place_object(player_ptr, y, x, 0L);
            break;
        }
    }
}
