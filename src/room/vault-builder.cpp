#include "room/vault-builder.h"
#include "floor/cave.h"
#include "floor/floor-generator-util.h"
#include "floor/floor-util.h"
#include "game-option/cheat-options.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"

/*
 * Grid based version of "creature_bold()"
 */
static bool player_grid(PlayerType *player_ptr, const Grid &grid)
{
    return &grid == &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
}

/*
 * Grid based version of "cave_empty_bold()"
 */
static bool is_cave_empty_grid(PlayerType *player_ptr, const Grid &grid)
{
    bool is_empty_grid = grid.has(TerrainCharacteristics::PLACE);
    is_empty_grid &= !grid.has_monster();
    is_empty_grid &= !player_grid(player_ptr, grid);
    return is_empty_grid;
}

/*!
 * @brief 特殊な部屋地形向けにモンスターを配置する / Place some sleeping monsters near the given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y1 モンスターを配置したいマスの中心Y座標
 * @param x1 モンスターを配置したいマスの中心X座標
 * @param num 配置したいモンスターの数
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_monsters(PlayerType *player_ptr, POSITION y1, POSITION x1, int num)
{
    auto &floor = *player_ptr->current_floor_ptr;
    for (auto k = 0; k < num; k++) {
        for (auto i = 0; i < 9; i++) {
            const auto d = 1;
            const auto pos = scatter(player_ptr, { y1, x1 }, d, 0);
            auto &grid = floor.get_grid(pos);
            if (!is_cave_empty_grid(player_ptr, grid)) {
                continue;
            }

            floor.monster_level = floor.base_level + 2;
            (void)place_random_monster(player_ptr, pos.y, pos.x, PM_ALLOW_SLEEP | PM_ALLOW_GROUP);
            floor.monster_level = floor.base_level;
        }
    }
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する / Create up to "num" objects near the given coordinates
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 配置したい中心マスのY座標
 * @param x 配置したい中心マスのX座標
 * @param num 配置したい数
 * @details
 * Only really called by some of the "vault" routines.
 */
void vault_objects(PlayerType *player_ptr, POSITION y, POSITION x, int num)
{
    const Pos2D pos_center(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    for (; num > 0; --num) {
        Pos2D pos = pos_center;
        int dummy = 0;
        for (int i = 0; i < 11; ++i) {
            while (dummy < SAFE_MAX_ATTEMPTS) {
                pos.y = rand_spread(pos_center.y, 2);
                pos.x = rand_spread(pos_center.x, 3);
                dummy++;
                if (!in_bounds(floor, pos.y, pos.x)) {
                    continue;
                }

                break;
            }

            if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room) {
                msg_print(_("警告！地下室のアイテムを配置できません！", "Warning! Could not place vault object!"));
            }

            const auto &grid = floor.get_grid(pos);
            if (!grid.is_floor() || !grid.o_idx_list.empty()) {
                continue;
            }

            if (evaluate_percent(75)) {
                place_object(player_ptr, pos.y, pos.x, 0);
            } else {
                place_gold(player_ptr, pos.y, pos.x);
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
static void vault_trap_aux(FloorType &floor, POSITION y, POSITION x, POSITION yd, POSITION xd)
{
    int y1 = y, x1 = x;
    int dummy = 0;
    for (int count = 0; count <= 5; count++) {
        while (dummy < SAFE_MAX_ATTEMPTS) {
            y1 = rand_spread(y, yd);
            x1 = rand_spread(x, xd);
            dummy++;
            if (!in_bounds(floor, y1, x1)) {
                continue;
            }
            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS && cheat_room) {
            msg_print(_("警告！地下室のトラップを配置できません！", "Warning! Could not place vault trap!"));
        }

        const auto &grid = floor.grid_array[y1][x1];
        if (!grid.is_floor() || !grid.o_idx_list.empty() || grid.has_monster()) {
            continue;
        }

        place_trap(floor, y1, x1);
        break;
    }
}

/*!
 * @brief 特殊な部屋向けに各種アイテムを配置する(メインルーチン) / Place some traps with a given displacement of given location
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y トラップを配置したいマスの中心Y座標
 * @param x トラップを配置したいマスの中心X座標
 * @param yd Y方向の配置分散マス数
 * @param xd X方向の配置分散マス数
 * @param num 配置したいトラップの数
 * @details
 * Only really called by some of the "vault" routines.
 * @todo rooms-normal からしか呼ばれていない、要調整
 */
void vault_traps(FloorType &floor, POSITION y, POSITION x, POSITION yd, POSITION xd, int num)
{
    for (int i = 0; i < num; i++) {
        vault_trap_aux(floor, y, x, yd, xd);
    }
}
