#include "floor/cave-generator.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest-monster-placer.h"
#include "floor/dungeon-tunnel-util.h"
#include "floor/floor-allocation-types.h"
#include "floor/floor-streams.h"
#include "floor/geometry.h"
#include "floor/object-allocator.h"
#include "floor/tunnel-generator.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "game-option/game-play-options.h"
#include "grid/door.h"
#include "grid/feature-generator.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-util.h"
#include "room/lake-types.h"
#include "room/room-generator.h"
#include "room/rooms-maze-vault.h"
#include "system/dungeon-data-definition.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"

static void reset_lite_area(floor_type *floor_ptr)
{
    floor_ptr->lite_n = 0;
    floor_ptr->mon_lite_n = 0;
    floor_ptr->redraw_n = 0;
    floor_ptr->view_n = 0;
}

static dun_data_type *initialize_dun_data_type(dun_data_type *dd_ptr, concptr *why)
{
    dd_ptr->destroyed = false;
    dd_ptr->empty_level = false;
    dd_ptr->cavern = false;
    dd_ptr->laketype = 0;
    dd_ptr->why = why;
    return dd_ptr;
}

static void check_arena_floor(player_type *player_ptr, dun_data_type *dd_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (!dd_ptr->empty_level) {
        for (POSITION y = 0; y < floor_ptr->height; y++)
            for (POSITION x = 0; x < floor_ptr->width; x++)
                place_bold(player_ptr, y, x, GB_EXTRA);

        return;
    }

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
}

static void place_cave_contents(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->dun_level == 1)
        while (one_in_(DUN_MOS_DEN))
            place_trees(player_ptr, randint1(floor_ptr->width - 2), randint1(floor_ptr->height - 2));

    if (dd_ptr->destroyed)
        destroy_level(player_ptr);

    if (has_river_flag(d_ptr) && one_in_(3) && (randint1(floor_ptr->dun_level) > 5))
        add_river(floor_ptr, dd_ptr);

    for (int i = 0; i < dd_ptr->cent_n; i++) {
        POSITION ty, tx;
        int pick = rand_range(0, i);
        ty = dd_ptr->cent[i].y;
        tx = dd_ptr->cent[i].x;
        dd_ptr->cent[i].y = dd_ptr->cent[pick].y;
        dd_ptr->cent[i].x = dd_ptr->cent[pick].x;
        dd_ptr->cent[pick].y = ty;
        dd_ptr->cent[pick].x = tx;
    }
}

static bool decide_tunnel_planned_site(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr, dt_type *dt_ptr, int i)
{
    dd_ptr->tunn_n = 0;
    dd_ptr->wall_n = 0;
    if (randint1(player_ptr->current_floor_ptr->dun_level) > d_ptr->tunnel_percent)
        (void)build_tunnel2(player_ptr, dd_ptr, dd_ptr->cent[i].x, dd_ptr->cent[i].y, dd_ptr->tunnel_x, dd_ptr->tunnel_y, 2, 2);
    else if (!build_tunnel(player_ptr, dd_ptr, dt_ptr, dd_ptr->cent[i].y, dd_ptr->cent[i].x, dd_ptr->tunnel_y, dd_ptr->tunnel_x))
        dd_ptr->tunnel_fail_count++;

    if (dd_ptr->tunnel_fail_count >= 2) {
        *dd_ptr->why = _("トンネル接続に失敗", "Failed to generate tunnels");
        return false;
    }

    return true;
}

static void make_tunnels(player_type *player_ptr, dun_data_type *dd_ptr)
{
    for (int j = 0; j < dd_ptr->tunn_n; j++) {
        grid_type *g_ptr;
        feature_type *f_ptr;
        dd_ptr->tunnel_y = dd_ptr->tunn[j].y;
        dd_ptr->tunnel_x = dd_ptr->tunn[j].x;
        g_ptr = &player_ptr->current_floor_ptr->grid_array[dd_ptr->tunnel_y][dd_ptr->tunnel_x];
        f_ptr = &f_info[g_ptr->feat];
        if (f_ptr->flags.has_not(FF::MOVE) || f_ptr->flags.has_none_of({FF::WATER, FF::LAVA})) {
            g_ptr->mimic = 0;
            place_grid(player_ptr, g_ptr, GB_FLOOR);
        }
    }
}

static void make_walls(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr, dt_type *dt_ptr)
{
    for (int j = 0; j < dd_ptr->wall_n; j++) {
        grid_type *g_ptr;
        dd_ptr->tunnel_y = dd_ptr->wall[j].y;
        dd_ptr->tunnel_x = dd_ptr->wall[j].x;
        g_ptr = &player_ptr->current_floor_ptr->grid_array[dd_ptr->tunnel_y][dd_ptr->tunnel_x];
        g_ptr->mimic = 0;
        place_grid(player_ptr, g_ptr, GB_FLOOR);
        if ((randint0(100) < dt_ptr->dun_tun_pen) && d_ptr->flags.has_not(DF::NO_DOORS))
            place_random_door(player_ptr, dd_ptr->tunnel_y, dd_ptr->tunnel_x, true);
    }
}

static bool make_centers(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr, dt_type *dt_ptr)
{
    dd_ptr->tunnel_fail_count = 0;
    dd_ptr->door_n = 0;
    dd_ptr->tunnel_y = dd_ptr->cent[dd_ptr->cent_n - 1].y;
    dd_ptr->tunnel_x = dd_ptr->cent[dd_ptr->cent_n - 1].x;
    for (int i = 0; i < dd_ptr->cent_n; i++) {
        if (!decide_tunnel_planned_site(player_ptr, dd_ptr, d_ptr, dt_ptr, i))
            return false;

        make_tunnels(player_ptr, dd_ptr);
        make_walls(player_ptr, dd_ptr, d_ptr, dt_ptr);
        dd_ptr->tunnel_y = dd_ptr->cent[i].y;
        dd_ptr->tunnel_x = dd_ptr->cent[i].x;
    }

    return true;
}

static void make_doors(player_type *player_ptr, dun_data_type *dd_ptr, dt_type *dt_ptr)
{
    for (int i = 0; i < dd_ptr->door_n; i++) {
        dd_ptr->tunnel_y = dd_ptr->door[i].y;
        dd_ptr->tunnel_x = dd_ptr->door[i].x;
        try_door(player_ptr, dt_ptr, dd_ptr->tunnel_y, dd_ptr->tunnel_x - 1);
        try_door(player_ptr, dt_ptr, dd_ptr->tunnel_y, dd_ptr->tunnel_x + 1);
        try_door(player_ptr, dt_ptr, dd_ptr->tunnel_y - 1, dd_ptr->tunnel_x);
        try_door(player_ptr, dt_ptr, dd_ptr->tunnel_y + 1, dd_ptr->tunnel_x);
    }
}

static void make_only_tunnel_points(floor_type *floor_ptr, dun_data_type *dd_ptr)
{
    int point_num = (floor_ptr->width * floor_ptr->height) / 200 + randint1(3);
    dd_ptr->cent_n = point_num;
    for (int i = 0; i < point_num; i++) {
        dd_ptr->cent[i].y = randint0(floor_ptr->height);
        dd_ptr->cent[i].x = randint0(floor_ptr->width);
    }
}

static bool make_one_floor(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;

    if (d_info[floor_ptr->dungeon_idx].flags.has(DF::NO_ROOM)) {
        make_only_tunnel_points(floor_ptr, dd_ptr);
    } else {
        if (!generate_rooms(player_ptr, dd_ptr)) {
            *dd_ptr->why = _("部屋群の生成に失敗", "Failed to generate rooms");
            return false;
        }
    }

    place_cave_contents(player_ptr, dd_ptr, d_ptr);
    dt_type tmp_dt;
    dt_type *dt_ptr = initialize_dt_type(&tmp_dt);
    if (!make_centers(player_ptr, dd_ptr, d_ptr, dt_ptr))
        return false;

    make_doors(player_ptr, dd_ptr, dt_ptr);
    if (!alloc_stairs(player_ptr, feat_down_stair, rand_range(3, 4), 3)) {
        *dd_ptr->why = _("下り階段生成に失敗", "Failed to generate down stairs.");
        return false;
    }

    if (!alloc_stairs(player_ptr, feat_up_stair, rand_range(1, 2), 3)) {
        *dd_ptr->why = _("上り階段生成に失敗", "Failed to generate up stairs.");
        return false;
    }

    return true;
}

static bool switch_making_floor(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr)
{
    if (d_ptr->flags.has(DF::MAZE)) {
        floor_type *floor_ptr = player_ptr->current_floor_ptr;
        build_maze_vault(player_ptr, floor_ptr->width / 2 - 1, floor_ptr->height / 2 - 1, floor_ptr->width - 4, floor_ptr->height - 4, false);
        if (!alloc_stairs(player_ptr, feat_down_stair, rand_range(2, 3), 3)) {
            *dd_ptr->why = _("迷宮ダンジョンの下り階段生成に失敗", "Failed to alloc up stairs in maze dungeon.");
            return false;
        }

        if (!alloc_stairs(player_ptr, feat_up_stair, 1, 3)) {
            *dd_ptr->why = _("迷宮ダンジョンの上り階段生成に失敗", "Failed to alloc down stairs in maze dungeon.");
            return false;
        }

        return true;
    }
    
    if (!make_one_floor(player_ptr, dd_ptr, d_ptr))
        return false;

    return true;
}

static void make_aqua_streams(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr)
{
    if (dd_ptr->laketype != 0)
        return;

    if (d_ptr->stream2)
        for (int i = 0; i < DUN_STR_QUA; i++)
            build_streamer(player_ptr, d_ptr->stream2, DUN_STR_QC);

    if (d_ptr->stream1)
        for (int i = 0; i < DUN_STR_MAG; i++)
            build_streamer(player_ptr, d_ptr->stream1, DUN_STR_MC);
}

/*!
 * @brief マスにフロア端用の永久壁を配置する / Set boundary mimic and add "solid" perma-wall
 * @param g_ptr 永久壁を配置したいマス構造体の参照ポインタ
 */
static void place_bound_perm_wall(player_type *player_ptr, grid_type *g_ptr)
{
    if (bound_walls_perm) {
        g_ptr->mimic = 0;
        place_grid(player_ptr, g_ptr, GB_SOLID_PERM);
        return;
    }

    auto *f_ptr = &f_info[g_ptr->feat];
    if (f_ptr->flags.has_any_of({FF::HAS_GOLD, FF::HAS_ITEM}) && f_ptr->flags.has_not(FF::SECRET)) {
        g_ptr->feat = feat_state(player_ptr->current_floor_ptr, g_ptr->feat, FF::ENSECRET);
    }

    g_ptr->mimic = g_ptr->feat;
    place_grid(player_ptr, g_ptr, GB_SOLID_PERM);
}

static void make_perm_walls(player_type *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION x = 0; x < floor_ptr->width; x++) {
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[0][x]);
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[floor_ptr->height - 1][x]);
    }

    for (POSITION y = 1; y < (floor_ptr->height - 1); y++) {
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[y][0]);
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[y][floor_ptr->width - 1]);
    }
}

static bool check_place_necessary_objects(player_type *player_ptr, dun_data_type *dd_ptr)
{
    if (!new_player_spot(player_ptr)) {
        *dd_ptr->why = _("プレイヤー配置に失敗", "Failed to place a player");
        return false;
    }

    if (!place_quest_monsters(player_ptr)) {
        *dd_ptr->why = _("クエストモンスター配置に失敗", "Failed to place a quest monster");
        return false;
    }

    return true;
}

static void decide_dungeon_data_allocation(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    dd_ptr->alloc_object_num = floor_ptr->dun_level / 3;
    if (dd_ptr->alloc_object_num > 10)
        dd_ptr->alloc_object_num = 10;

    if (dd_ptr->alloc_object_num < 2)
        dd_ptr->alloc_object_num = 2;

    dd_ptr->alloc_monster_num = d_ptr->min_m_alloc_level;
    if (floor_ptr->height >= MAX_HGT && floor_ptr->width >= MAX_WID)
        return;

    int small_tester = dd_ptr->alloc_monster_num;
    dd_ptr->alloc_monster_num = (dd_ptr->alloc_monster_num * floor_ptr->height) / MAX_HGT;
    dd_ptr->alloc_monster_num = (dd_ptr->alloc_monster_num * floor_ptr->width) / MAX_WID;
    dd_ptr->alloc_monster_num += 1;
    if (dd_ptr->alloc_monster_num > small_tester)
        dd_ptr->alloc_monster_num = small_tester;
    else
        msg_format_wizard(player_ptr, CHEAT_DUNGEON, _("モンスター数基本値を %d から %d に減らします", "Reduced monsters base from %d to %d"), small_tester,
            dd_ptr->alloc_monster_num);
}

static bool allocate_dungeon_data(player_type *player_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr)
{
    dd_ptr->alloc_monster_num += randint1(8);
    for (dd_ptr->alloc_monster_num = dd_ptr->alloc_monster_num + dd_ptr->alloc_object_num; dd_ptr->alloc_monster_num > 0; dd_ptr->alloc_monster_num--)
        (void)alloc_monster(player_ptr, 0, PM_ALLOW_SLEEP, summon_specific);

    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(dd_ptr->alloc_object_num));
    if (d_ptr->flags.has_not(DF::NO_CAVE))
        alloc_object(player_ptr, ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint1(dd_ptr->alloc_object_num));

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (player_ptr->enter_dungeon && floor_ptr->dun_level > 1)
        floor_ptr->object_level = 1;

    alloc_object(player_ptr, ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3));
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3));
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3));
    floor_ptr->object_level = floor_ptr->base_level;
    if (alloc_guardian(player_ptr, true))
        return true;

    *dd_ptr->why = _("ダンジョンの主配置に失敗", "Failed to place a dungeon guardian");
    return false;
}

static void decide_grid_glowing(floor_type *floor_ptr, dun_data_type *dd_ptr, dungeon_type *d_ptr)
{
    bool is_empty_or_dark = dd_ptr->empty_level;
    is_empty_or_dark &= !one_in_(DARK_EMPTY) || (randint1(100) > floor_ptr->dun_level);
    is_empty_or_dark &= d_ptr->flags.has_not(DF::DARKNESS);
    if (!is_empty_or_dark)
        return;

    for (POSITION y = 0; y < floor_ptr->height; y++)
        for (POSITION x = 0; x < floor_ptr->width; x++)
            floor_ptr->grid_array[y][x].info |= CAVE_GLOW;
}

/*!
 * @brief ダンジョン生成のメインルーチン / Generate a new dungeon level
 * @details Note that "dun_body" adds about 4000 bytes of memory to the stack.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param why エラー原因メッセージを返す
 * @return ダンジョン生成が全て無事に成功したらTRUEを返す。
 */
bool cave_gen(player_type *player_ptr, concptr *why)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    reset_lite_area(floor_ptr);
    set_floor_and_wall(floor_ptr->dungeon_idx);
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), nullptr);

    dun_data_type tmp_dd;
    dun_data_type *dd_ptr = initialize_dun_data_type(&tmp_dd, why);
    dd_ptr->row_rooms = floor_ptr->height / BLOCK_HGT;
    dd_ptr->col_rooms = floor_ptr->width / BLOCK_WID;
    for (POSITION y = 0; y < dd_ptr->row_rooms; y++)
        for (POSITION x = 0; x < dd_ptr->col_rooms; x++)
            dd_ptr->room_map[y][x] = false;

    dd_ptr->cent_n = 0;
    dungeon_type *d_ptr = &d_info[floor_ptr->dungeon_idx];
    if (ironman_empty_levels || (d_ptr->flags.has(DF::ARENA) && (empty_levels && one_in_(EMPTY_LEVEL)))) {
        dd_ptr->empty_level = true;
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("アリーナレベルを生成。", "Arena level."));
    }

    check_arena_floor(player_ptr, dd_ptr);
    gen_caverns_and_lakes(player_ptr, d_ptr, dd_ptr);
    if (!switch_making_floor(player_ptr, dd_ptr, d_ptr))
        return false;

    make_aqua_streams(player_ptr, dd_ptr, d_ptr);
    make_perm_walls(player_ptr);
    if (!check_place_necessary_objects(player_ptr, dd_ptr))
        return false;

    decide_dungeon_data_allocation(player_ptr, dd_ptr, d_ptr);
    if (!allocate_dungeon_data(player_ptr, dd_ptr, d_ptr))
        return false;

    decide_grid_glowing(floor_ptr, dd_ptr, d_ptr);
    return true;
}
