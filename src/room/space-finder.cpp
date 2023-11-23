#include "room/space-finder.h"
#include "dungeon/dungeon-flag-types.h"
#include "floor/cave.h"
#include "grid/grid.h"
#include "system/dungeon-data-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"

/*!
 * @brief 指定のマスが床系地形であるかを返す
 * @param pos チェックするマスの座標
 * @return 床系地形か否か
 */
static bool get_is_floor(FloorType *floor_ptr, const Pos2D &pos)
{
    if (!in_bounds(floor_ptr, pos.y, pos.x)) {
        return false;
    }

    if (floor_ptr->get_grid(pos).is_floor()) {
        return true;
    }

    return false;
}

/*!
 * @brief 指定のマスを床地形に変える
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 地形を変えたいマスの座標
 */
static void set_floor(PlayerType *player_ptr, const Pos2D &pos)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!in_bounds(floor_ptr, pos.y, pos.x)) {
        return;
    }

    auto &grid = floor_ptr->get_grid(pos);
    if (grid.is_room()) {
        return;
    }

    if (grid.is_extra()) {
        place_bold(player_ptr, pos.y, pos.x, GB_FLOOR);
    }
}

/*!
 * @brief 指定範囲に通路が通っていることを確認した上で床で埋める
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos1 範囲の左上端
 * @param pos2 範囲の右下端
 */
static void check_room_boundary(PlayerType *player_ptr, const Pos2D &pos1, const Pos2D &pos2)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto count = 0;
    auto old_is_floor = get_is_floor(floor_ptr, { pos1.y, pos1.x - 1 });
    bool new_is_floor;
    for (auto x = pos1.x; x <= pos2.x; x++) {
        new_is_floor = get_is_floor(floor_ptr, { pos1.y - 1, x });
        if (new_is_floor != old_is_floor) {
            count++;
        }

        old_is_floor = new_is_floor;
    }

    for (auto y = pos1.y; y <= pos2.y; y++) {
        new_is_floor = get_is_floor(floor_ptr, { y, pos2.x + 1 });
        if (new_is_floor != old_is_floor) {
            count++;
        }

        old_is_floor = new_is_floor;
    }

    for (auto x = pos2.x; x >= pos1.x; x--) {
        new_is_floor = get_is_floor(floor_ptr, { pos2.y + 1, x });
        if (new_is_floor != old_is_floor) {
            count++;
        }

        old_is_floor = new_is_floor;
    }

    for (auto y = pos2.y; y >= pos1.y; y--) {
        new_is_floor = get_is_floor(floor_ptr, { y, pos1.x - 1 });
        if (new_is_floor != old_is_floor) {
            count++;
        }

        old_is_floor = new_is_floor;
    }

    if (count <= 2) {
        return;
    }

    for (auto y = pos1.y; y <= pos2.y; y++) {
        for (auto x = pos1.x; x <= pos2.x; x++) {
            set_floor(player_ptr, { y, x });
        }
    }
}

/*!
 * @brief find_space()の予備処理として部屋の生成が可能かを判定する
 * @param blocks_high 範囲の高さ
 * @param blocks_wide 範囲の幅
 * @param block_y 範囲の上端
 * @param block_x 範囲の左端
 */
static bool find_space_aux(dun_data_type *dd_ptr, const Pos2D &max_block_size, const Pos2D &block)
{
    if (max_block_size.x < 3) {
        if ((max_block_size.x == 2) && (block.x % 3) == 2) {
            return false;
        }
    } else if ((max_block_size.x % 3) == 0) {
        if ((block.x % 3) != 0) {
            return false;
        }
    } else {
        if (block.x + (max_block_size.x / 2) <= dd_ptr->col_rooms / 2) {
            if (((block.x % 3) == 2) && ((max_block_size.x % 3) == 2)) {
                return false;
            }
            if ((block.x % 3) == 1) {
                return false;
            }
        } else {
            if (((block.x % 3) == 2) && ((max_block_size.x % 3) == 2)) {
                return false;
            }
            if ((block.x % 3) == 1) {
                return false;
            }
        }
    }

    const auto by1 = block.y;
    const auto bx1 = block.x;
    const auto by2 = block.y + max_block_size.y;
    const auto bx2 = block.x + max_block_size.x;
    if ((by1 < 0) || (by2 > dd_ptr->row_rooms) || (bx1 < 0) || (bx2 > dd_ptr->col_rooms)) {
        return false;
    }

    for (auto by = by1; by < by2; by++) {
        for (auto bx = bx1; bx < bx2; bx++) {
            if (dd_ptr->room_map[by][bx]) {
                return false;
            }
        }
    }

    return true;
}

/*!
 * @brief 部屋生成が可能なスペースを確保する / Find a good spot for the next room.  -LM-
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 部屋の生成が可能な中心Y座標を返す参照ポインタ
 * @param x 部屋の生成が可能な中心X座標を返す参照ポインタ
 * @param height 確保したい領域の高さ
 * @param width 確保したい領域の幅
 * @return 所定の範囲が確保できた場合TRUEを返す
 * @details
 * Find and allocate a free space in the dungeon large enough to hold\n
 * the room calling this function.\n
 *\n
 * We allocate space in 11x11 blocks, but want to make sure that rooms\n
 * alignment neatly on the standard screen.  Therefore, we make them use\n
 * blocks in few 11x33 rectangles as possible.\n
 *\n
 * Be careful to include the edges of the room in height and width!\n
 *\n
 * Return TRUE and values for the center of the room if all went well.\n
 * Otherwise, return FALSE.\n
 */
bool find_space(PlayerType *player_ptr, dun_data_type *dd_ptr, POSITION *y, POSITION *x, POSITION height, POSITION width)
{
    int pick;
    POSITION block_y = 0;
    POSITION block_x = 0;
    POSITION blocks_high = 1 + ((height - 1) / BLOCK_HGT);
    POSITION blocks_wide = 1 + ((width - 1) / BLOCK_WID);
    if ((dd_ptr->row_rooms < blocks_high) || (dd_ptr->col_rooms < blocks_wide)) {
        return false;
    }

    int candidates = 0;
    for (block_y = dd_ptr->row_rooms - blocks_high; block_y >= 0; block_y--) {
        for (block_x = dd_ptr->col_rooms - blocks_wide; block_x >= 0; block_x--) {
            if (find_space_aux(dd_ptr, { blocks_high, blocks_wide }, { block_y, block_x })) {
                /* Find a valid place */
                candidates++;
            }
        }
    }

    if (!candidates) {
        return false;
    }

    if (player_ptr->current_floor_ptr->get_dungeon_definition().flags.has_not(DungeonFeatureType::NO_CAVE)) {
        pick = randint1(candidates);
    } else {
        pick = candidates / 2 + 1;
    }

    for (block_y = dd_ptr->row_rooms - blocks_high; block_y >= 0; block_y--) {
        for (block_x = dd_ptr->col_rooms - blocks_wide; block_x >= 0; block_x--) {
            if (find_space_aux(dd_ptr, { blocks_high, blocks_wide }, { block_y, block_x })) {
                pick--;
                if (!pick) {
                    break;
                }
            }
        }

        if (!pick) {
            break;
        }
    }

    POSITION by1 = block_y;
    POSITION bx1 = block_x;
    POSITION by2 = block_y + blocks_high;
    POSITION bx2 = block_x + blocks_wide;
    *y = ((by1 + by2) * BLOCK_HGT) / 2;
    *x = ((bx1 + bx2) * BLOCK_WID) / 2;
    if (dd_ptr->cent_n < CENT_MAX) {
        dd_ptr->cent[dd_ptr->cent_n].y = (byte)*y;
        dd_ptr->cent[dd_ptr->cent_n].x = (byte)*x;
        dd_ptr->cent_n++;
    }

    for (POSITION by = by1; by < by2; by++) {
        for (POSITION bx = bx1; bx < bx2; bx++) {
            if ((by < 0) || (bx < 0)) {
                continue;
            }

            dd_ptr->room_map[by][bx] = true;
        }
    }

    check_room_boundary(player_ptr, { *y - height / 2 - 1, *x - width / 2 - 1 }, { *y + (height - 1) / 2 + 1, *x + (width - 1) / 2 + 1 });
    return true;
}
