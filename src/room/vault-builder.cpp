#include "room/vault-builder.h"
#include "floor/cave.h"
#include "floor/floor-generator-util.h"
#include "floor/floor-util.h"
#include "game-option/cheat-options.h"
#include "grid/feature-flag-types.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*
 * Grid based version of "creature_bold()"
 */
static bool player_grid(player_type *player_ptr, grid_type *g_ptr) { return g_ptr == &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x]; }

/*
 * Grid based version of "cave_empty_bold()"
 */
static bool is_cave_empty_grid(player_type *player_ptr, grid_type *g_ptr)
{
    bool is_empty_grid = g_ptr->cave_has_flag(FF::PLACE);
    is_empty_grid &= g_ptr->m_idx == 0;
    is_empty_grid &= !player_grid(player_ptr, g_ptr);
    return is_empty_grid;
}

/*!
 * @brief 特殊な部屋地形向けにモンスターを配置する / Place some sleeping monsters near the given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y1 モンスターを配置したいマスの中心Y座標
 * @param x1 モンスターを配置したいマスの中心X座標
 * @param num 配置したいモンスターの数
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_monsters(player_type *player_ptr, POSITION y1, POSITION x1, int num)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int k = 0; k < num; k++) {
        for (int i = 0; i < 9; i++) {
            int d = 1;
            POSITION y, x;
            scatter(player_ptr, &y, &x, y1, x1, d, 0);
            grid_type *g_ptr;
            g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];
            if (!is_cave_empty_grid(player_ptr, g_ptr))
                continue;

            floor_ptr->monster_level = floor_ptr->base_level + 2;
            (void)place_monster(player_ptr, y, x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
            floor_ptr->monster_level = floor_ptr->base_level;
        }
    }
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する / Create up to "num" objects near the given coordinates
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 配置したい中心マスのY座標
 * @param x 配置したい中心マスのX座標
 * @param num 配置したい数
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_objects(player_type *player_ptr, POSITION y, POSITION x, int num)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (; num > 0; --num) {
        int j = y, k = x;
        int dummy = 0;
        for (int i = 0; i < 11; ++i) {
            while (dummy < SAFE_MAX_ATTEMPTS) {
                j = rand_spread(y, 2);
                k = rand_spread(x, 3);
                dummy++;
                if (!in_bounds(floor_ptr, j, k))
                    continue;
                break;
            }

            if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room) {
                msg_print(_("警告！地下室のアイテムを配置できません！", "Warning! Could not place vault object!"));
            }

            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[j][k];
            if (!g_ptr->is_floor() || !g_ptr->o_idx_list.empty())
                continue;

            if (randint0(100) < 75) {
                place_object(player_ptr, j, k, 0L);
            } else {
                place_gold(player_ptr, j, k);
            }

            break;
        }
    }
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する(vault_trapのサブセット) / Place a trap with a given displacement of point
 * @param y トラップを配置したいマスの中心Y座標
 * @param x トラップを配置したいマスの中心X座標
 * @param yd Y方向の配置分散マス数
 * @param xd X方向の配置分散マス数
 * @details
 * Only really called by some of the "vault" routines.
 */
static void vault_trap_aux(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd)
{
    grid_type *g_ptr;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    int y1 = y, x1 = x;
    int dummy = 0;
    for (int count = 0; count <= 5; count++) {
        while (dummy < SAFE_MAX_ATTEMPTS) {
            y1 = rand_spread(y, yd);
            x1 = rand_spread(x, xd);
            dummy++;
            if (!in_bounds(floor_ptr, y1, x1))
                continue;
            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room) {
            msg_print(_("警告！地下室のトラップを配置できません！", "Warning! Could not place vault trap!"));
        }

        g_ptr = &floor_ptr->grid_array[y1][x1];
        if (!g_ptr->is_floor() || !g_ptr->o_idx_list.empty() || g_ptr->m_idx)
            continue;

        place_trap(player_ptr, y1, x1);
        break;
    }
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する(メインルーチン) / Place some traps with a given displacement of given location
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y トラップを配置したいマスの中心Y座標
 * @param x トラップを配置したいマスの中心X座標
 * @param yd Y方向の配置分散マス数
 * @param xd X方向の配置分散マス数
 * @param num 配置したいトラップの数
 * @details
 * Only really called by some of the "vault" routines.
 * @todo rooms-normal からしか呼ばれていない、要調整
 */
void vault_traps(player_type *player_ptr, POSITION y, POSITION x, POSITION yd, POSITION xd, int num)
{
    for (int i = 0; i < num; i++)
        vault_trap_aux(player_ptr, y, x, yd, xd);
}
