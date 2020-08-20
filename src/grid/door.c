#include "grid/door.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "floor/cave.h"
#include "floor/floor.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"

/*
 * Routine used by the random vault creators to add a door to a location
 * Note that range checking has to be done in the calling routine.
 *
 * The doors must be INSIDE the allocated region.
 */
void add_door(player_type *player_ptr, POSITION x, POSITION y)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!is_outer_bold(floor_ptr, y, x))
        return;

    /* look at:
     *  x#x
     *  .#.
     *  x#x
     *
     *  where x=don't care
     *  .=floor, #=wall
     */

    if (is_floor_bold(floor_ptr, y - 1, x) && is_floor_bold(floor_ptr, y + 1, x)
        && (is_outer_bold(floor_ptr, y, x - 1) && is_outer_bold(floor_ptr, y, x + 1))) {
        place_secret_door(player_ptr, y, x, DOOR_DEFAULT);
        place_bold(player_ptr, y, x - 1, GB_SOLID);
        place_bold(player_ptr, y, x + 1, GB_SOLID);
    }

    /* look at:
     *  x#x
     *  .#.
     *  x#x
     *
     *  where x = don't care
     *  .=floor, #=wall
     */
    if (is_outer_bold(floor_ptr, y - 1, x) && is_outer_bold(floor_ptr, y + 1, x) && is_floor_bold(floor_ptr, y, x - 1) && is_floor_bold(floor_ptr, y, x + 1)) {
        place_secret_door(player_ptr, y, x, DOOR_DEFAULT);
        place_bold(player_ptr, y - 1, x, GB_SOLID);
        place_bold(player_ptr, y + 1, x, GB_SOLID);
    }
}

/*!
 * @brief 隠しドアを配置する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @param type DOOR_DEFAULT / DOOR_DOOR / DOOR_GLASS_DOOR / DOOR_CURTAIN のいずれか
 * @return なし
 */
void place_secret_door(player_type *player_ptr, POSITION y, POSITION x, int type)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_DOORS) {
        place_bold(player_ptr, y, x, GB_FLOOR);
        return;
    }

    if (type == DOOR_DEFAULT) {
        type = ((d_info[floor_ptr->dungeon_idx].flags1 & DF1_CURTAIN) && one_in_((d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_CAVE) ? 16 : 256))
            ? DOOR_CURTAIN
            : ((d_info[floor_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR);
    }

    place_closed_door(player_ptr, y, x, type);
    grid_type *g_ptr = &floor_ptr->grid_array[y][x];
    if (type != DOOR_CURTAIN) {
        g_ptr->mimic = feat_wall_inner;
        if (feat_supports_los(g_ptr->mimic) && !feat_supports_los(g_ptr->feat)) {
            if (have_flag(f_info[g_ptr->mimic].flags, FF_MOVE) || have_flag(f_info[g_ptr->mimic].flags, FF_CAN_FLY)) {
                g_ptr->feat = one_in_(2) ? g_ptr->mimic : feat_ground_type[randint0(100)];
            }

            g_ptr->mimic = 0;
        }
    }

    g_ptr->info &= ~(CAVE_FLOOR);
    delete_monster(player_ptr, y, x);
}

/*!
 * @brief 鍵のかかったドアを配置する
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param y 配置したいフロアのY座標
 * @param x 配置したいフロアのX座標
 * @return なし
 */
void place_locked_door(player_type *player_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (d_info[floor_ptr->dungeon_idx].flags1 & DF1_NO_DOORS) {
        place_bold(player_ptr, y, x, GB_FLOOR);
        return;
    }

    set_cave_feat(floor_ptr, y, x, feat_locked_door_random((d_info[player_ptr->dungeon_idx].flags1 & DF1_GLASS_DOOR) ? DOOR_GLASS_DOOR : DOOR_DOOR));
    floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
    delete_monster(player_ptr, y, x);
}
