#include "floor/cave-generator.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest-monster-placer.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/dungeon-tunnel-util.h"
#include "floor/floor-allocation-types.h"
#include "floor/floor-streams.h"
#include "floor/tunnel-generator.h"
#include "floor/floor.h"
#include "floor/geometry.h"
#include "floor/object-allocator.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "game-option/game-play-options.h"
#include "grid/feature-generator.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "room/lake-types.h"
#include "room/rooms-builder.h"
#include "room/room-generator.h"
#include "room/rooms-maze-vault.h"
#include "system/dungeon-data-definition.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief ダンジョン生成のメインルーチン / Generate a new dungeon level
 * @details Note that "dun_body" adds about 4000 bytes of memory to the stack.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param why エラー原因メッセージを返す
 * @return ダンジョン生成が全て無事に成功したらTRUEを返す。
 */
bool cave_gen(player_type *player_ptr, concptr *why)
{
    int i, k;
    dun_data_type dun_body;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    dungeon_type *dungeon_ptr = &d_info[floor_ptr->dungeon_idx];
    floor_ptr->lite_n = 0;
    floor_ptr->mon_lite_n = 0;
    floor_ptr->redraw_n = 0;
    floor_ptr->view_n = 0;
    dun_data = &dun_body;
    dun_data->destroyed = FALSE;
    dun_data->empty_level = FALSE;
    dun_data->cavern = FALSE;
    dun_data->laketype = 0;
    set_floor_and_wall(floor_ptr->dungeon_idx);
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), NULL);

    dt_type tmp_dt;
    dt_type *dt_ptr = initialize_dt_type(&tmp_dt);

    dun_data->row_rooms = floor_ptr->height / BLOCK_HGT;
    dun_data->col_rooms = floor_ptr->width / BLOCK_WID;
    for (POSITION y = 0; y < dun_data->row_rooms; y++)
        for (POSITION x = 0; x < dun_data->col_rooms; x++)
            dun_data->room_map[y][x] = FALSE;

    dun_data->cent_n = 0;
    if (ironman_empty_levels || ((dungeon_ptr->flags1 & DF1_ARENA) && (empty_levels && one_in_(EMPTY_LEVEL)))) {
        dun_data->empty_level = TRUE;
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("アリーナレベルを生成。", "Arena level."));
    }

    if (dun_data->empty_level) {
        for (POSITION y = 0; y < floor_ptr->height; y++)
            for (POSITION x = 0; x < floor_ptr->width; x++)
                place_bold(player_ptr, y, x, GB_FLOOR);

        for (POSITION x = 0; x < floor_ptr->width; x++) {
            place_bold(player_ptr, 0, x, GB_EXTRA);
            place_bold(player_ptr, floor_ptr->height - 1, x, GB_EXTRA);
        }

        for (POSITION y = 1; y < (floor_ptr->height - 1); y++) {
            place_bold(player_ptr, y, 0, GB_EXTRA);
            place_bold(player_ptr, y, floor_ptr->width - 1, GB_EXTRA);
        }
    } else {
        for (POSITION y = 0; y < floor_ptr->height; y++)
            for (POSITION x = 0; x < floor_ptr->width; x++)
                place_bold(player_ptr, y, x, GB_EXTRA);
    }

    gen_caverns_and_lakes(dungeon_ptr, player_ptr);
    if (dungeon_ptr->flags1 & DF1_MAZE) {
        build_maze_vault(player_ptr, floor_ptr->width / 2 - 1, floor_ptr->height / 2 - 1, floor_ptr->width - 4, floor_ptr->height - 4, FALSE);
        if (!alloc_stairs(player_ptr, feat_down_stair, rand_range(2, 3), 3)) {
            *why = _("迷宮ダンジョンの下り階段生成に失敗", "Failed to alloc up stairs in maze dungeon.");
            return FALSE;
        }

        if (!alloc_stairs(player_ptr, feat_up_stair, 1, 3)) {
            *why = _("迷宮ダンジョンの上り階段生成に失敗", "Failed to alloc down stairs in maze dungeon.");
            return FALSE;
        }
    } else {
        int tunnel_fail_count = 0;
        if (!generate_rooms(player_ptr)) {
            *why = _("部屋群の生成に失敗", "Failed to generate rooms");
            return FALSE;
        }

        if (floor_ptr->dun_level == 1)
            while (one_in_(DUN_MOS_DEN))
                place_trees(player_ptr, randint1(floor_ptr->width - 2), randint1(floor_ptr->height - 2));

        if (dun_data->destroyed)
            destroy_level(player_ptr);

        if (has_river_flag(dungeon_ptr) && one_in_(3) && (randint1(floor_ptr->dun_level) > 5))
            add_river(floor_ptr);

        for (i = 0; i < dun_data->cent_n; i++) {
            POSITION ty, tx;
            int pick = rand_range(0, i);
            ty = dun_data->cent[i].y;
            tx = dun_data->cent[i].x;
            dun_data->cent[i].y = dun_data->cent[pick].y;
            dun_data->cent[i].x = dun_data->cent[pick].x;
            dun_data->cent[pick].y = ty;
            dun_data->cent[pick].x = tx;
        }

        dun_data->door_n = 0;
        POSITION y = dun_data->cent[dun_data->cent_n - 1].y;
        POSITION x = dun_data->cent[dun_data->cent_n - 1].x;

        for (i = 0; i < dun_data->cent_n; i++) {
            dun_data->tunn_n = 0;
            dun_data->wall_n = 0;
            if (randint1(floor_ptr->dun_level) > dungeon_ptr->tunnel_percent)
                (void)build_tunnel2(player_ptr, dun_data->cent[i].x, dun_data->cent[i].y, x, y, 2, 2);
            else if (!build_tunnel(player_ptr, dt_ptr, dun_data->cent[i].y, dun_data->cent[i].x, y, x))
                tunnel_fail_count++;

            if (tunnel_fail_count >= 2) {
                *why = _("トンネル接続に失敗", "Failed to generate tunnels");
                return FALSE;
            }

            for (int j = 0; j < dun_data->tunn_n; j++) {
                grid_type *g_ptr;
                feature_type *f_ptr;
                y = dun_data->tunn[j].y;
                x = dun_data->tunn[j].x;
                g_ptr = &floor_ptr->grid_array[y][x];
                f_ptr = &f_info[g_ptr->feat];

                if (!have_flag(f_ptr->flags, FF_MOVE) || (!have_flag(f_ptr->flags, FF_WATER) && !have_flag(f_ptr->flags, FF_LAVA))) {
                    g_ptr->mimic = 0;
                    place_grid(player_ptr, g_ptr, GB_FLOOR);
                }
            }

            for (int j = 0; j < dun_data->wall_n; j++) {
                grid_type *g_ptr;
                y = dun_data->wall[j].y;
                x = dun_data->wall[j].x;
                g_ptr = &floor_ptr->grid_array[y][x];
                g_ptr->mimic = 0;
                place_grid(player_ptr, g_ptr, GB_FLOOR);
                if ((randint0(100) < dt_ptr->dun_tun_pen) && !(dungeon_ptr->flags1 & DF1_NO_DOORS))
                    place_random_door(player_ptr, y, x, TRUE);
            }

            y = dun_data->cent[i].y;
            x = dun_data->cent[i].x;
        }

        for (i = 0; i < dun_data->door_n; i++) {
            y = dun_data->door[i].y;
            x = dun_data->door[i].x;
            try_door(player_ptr, dt_ptr, y, x - 1);
            try_door(player_ptr, dt_ptr, y, x + 1);
            try_door(player_ptr, dt_ptr, y - 1, x);
            try_door(player_ptr, dt_ptr, y + 1, x);
        }

        if (!alloc_stairs(player_ptr, feat_down_stair, rand_range(3, 4), 3)) {
            *why = _("下り階段生成に失敗", "Failed to generate down stairs.");
            return FALSE;
        }

        if (!alloc_stairs(player_ptr, feat_up_stair, rand_range(1, 2), 3)) {
            *why = _("上り階段生成に失敗", "Failed to generate up stairs.");
            return FALSE;
        }
    }

    if (!dun_data->laketype) {
        if (dungeon_ptr->stream2)
            for (i = 0; i < DUN_STR_QUA; i++)
                build_streamer(player_ptr, dungeon_ptr->stream2, DUN_STR_QC);

        if (dungeon_ptr->stream1)
            for (i = 0; i < DUN_STR_MAG; i++)
                build_streamer(player_ptr, dungeon_ptr->stream1, DUN_STR_MC);
    }

    for (POSITION x = 0; x < floor_ptr->width; x++) {
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[0][x]);
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[floor_ptr->height - 1][x]);
    }

    for (POSITION y = 1; y < (floor_ptr->height - 1); y++) {
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[y][0]);
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[y][floor_ptr->width - 1]);
    }

    if (!new_player_spot(player_ptr)) {
        *why = _("プレイヤー配置に失敗", "Failed to place a player");
        return FALSE;
    }

    if (!place_quest_monsters(player_ptr)) {
        *why = _("クエストモンスター配置に失敗", "Failed to place a quest monster");
        return FALSE;
    }

    k = (floor_ptr->dun_level / 3);
    if (k > 10)
        k = 10;
    if (k < 2)
        k = 2;

    i = dungeon_ptr->min_m_alloc_level;
    if (floor_ptr->height < MAX_HGT || floor_ptr->width < MAX_WID) {
        int small_tester = i;

        i = (i * floor_ptr->height) / MAX_HGT;
        i = (i * floor_ptr->width) / MAX_WID;
        i += 1;

        if (i > small_tester)
            i = small_tester;
        else
            msg_format_wizard(
                player_ptr, CHEAT_DUNGEON, _("モンスター数基本値を %d から %d に減らします", "Reduced monsters base from %d to %d"), small_tester, i);
    }

    i += randint1(8);

    for (i = i + k; i > 0; i--)
        (void)alloc_monster(player_ptr, 0, PM_ALLOW_SLEEP, summon_specific);

    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));
    if (!(dungeon_ptr->flags1 & DF1_NO_CAVE))
        alloc_object(player_ptr, ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint1(k));

    if (player_ptr->enter_dungeon && floor_ptr->dun_level > 1)
        floor_ptr->object_level = 1;

    alloc_object(player_ptr, ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3));
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3));
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3));
    floor_ptr->object_level = floor_ptr->base_level;
    if (!alloc_guardian(player_ptr, TRUE)) {
        *why = _("ダンジョンの主配置に失敗", "Failed to place a dungeon guardian");
        return FALSE;
    }

    bool is_empty_or_dark = dun_data->empty_level;
    is_empty_or_dark &= !one_in_(DARK_EMPTY) || (randint1(100) > floor_ptr->dun_level);
    is_empty_or_dark &= (dungeon_ptr->flags1 & DF1_DARKNESS) == 0;
    if (!is_empty_or_dark)
        return TRUE;

    for (POSITION y = 0; y < floor_ptr->height; y++)
        for (POSITION x = 0; x < floor_ptr->width; x++)
            floor_ptr->grid_array[y][x].info |= (CAVE_GLOW);

    return TRUE;
}
