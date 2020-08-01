/*!
 * @file generate.c
 * @brief ダンジョンの生成 / Dungeon generation
 * @date 2014/01/04
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 */

#include "floor/floor-generate.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-dump.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-events.h"
#include "floor/floor-generate.h"
#include "floor/floor-save.h"
#include "floor/floor-streams.h"
#include "floor/floor.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "game-option/game-play-options.h"
#include "game-option/play-record-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/trap.h"
#include "info-reader/feature-reader.h"
#include "info-reader/fixed-map-parser.h"
#include "io/write-diary.h"
#include "market/arena-info-table.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "player/player-status.h"
#include "room/lake-types.h"
#include "room/rooms-builder.h"
#include "room/room-generator.h"
#include "room/rooms-maze-vault.h"
#include "system/dungeon-data-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

int dun_tun_rnd;
int dun_tun_chg;
int dun_tun_con;
int dun_tun_pen;
int dun_tun_jct;

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

    return (k);
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

    /* Require "naked" floor grid */
    if (!is_floor_grid(g_ptr))
        return FALSE;
    if (pattern_tile(floor_ptr, y, x))
        return FALSE;
    if (g_ptr->o_idx || g_ptr->m_idx)
        return FALSE;

    /* Require a certain number of adjacent walls */
    if (next_to_walls(floor_ptr, y, x) < walls)
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
static bool alloc_stairs(player_type *owner_ptr, FEAT_IDX feat, int num, int walls)
{
    int i;
    int shaft_num = 0;

    feature_type *f_ptr = &f_info[feat];

    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    if (have_flag(f_ptr->flags, FF_LESS)) {
        /* No up stairs in town or in ironman mode */
        if (ironman_downward || !floor_ptr->dun_level)
            return TRUE;

        if (floor_ptr->dun_level > d_info[floor_ptr->dungeon_idx].mindepth)
            shaft_num = (randint1(num + 1)) / 2;
    } else if (have_flag(f_ptr->flags, FF_MORE)) {
        QUEST_IDX q_idx = quest_number(owner_ptr, floor_ptr->dun_level);

        /* No downstairs on quest levels */
        if (floor_ptr->dun_level > 1 && q_idx) {
            monster_race *r_ptr = &r_info[quest[q_idx].r_idx];

            /* The quest monster(s) is still alive? */
            if (!(r_ptr->flags1 & RF1_UNIQUE) || 0 < r_ptr->max_num)
                return TRUE;
        }

        /* No downstairs at the bottom */
        if (floor_ptr->dun_level >= d_info[floor_ptr->dungeon_idx].maxdepth)
            return TRUE;

        if ((floor_ptr->dun_level < d_info[floor_ptr->dungeon_idx].maxdepth - 1) && !quest_number(owner_ptr, floor_ptr->dun_level + 1))
            shaft_num = (randint1(num) + 1) / 2;
    } else
        return FALSE;

    /* Place "num" stairs */
    for (i = 0; i < num; i++) {
        while (TRUE) {
            POSITION y, x = 0;
            grid_type *g_ptr;

            int candidates = 0;
            int pick;

            for (y = 1; y < floor_ptr->height - 1; y++) {
                for (x = 1; x < floor_ptr->width - 1; x++) {
                    if (alloc_stairs_aux(owner_ptr, y, x, walls)) {
                        /* A valid space found */
                        candidates++;
                    }
                }
            }

            /* No valid place! */
            if (!candidates) {
                /* There are exactly no place! */
                if (walls <= 0)
                    return FALSE;

                /* Decrease walls limit, and try again */
                walls--;
                continue;
            }

            /* Choose a random one */
            pick = randint1(candidates);

            for (y = 1; y < floor_ptr->height - 1; y++) {
                for (x = 1; x < floor_ptr->width - 1; x++) {
                    if (alloc_stairs_aux(owner_ptr, y, x, walls)) {
                        pick--;

                        /* Is this a picked one? */
                        if (!pick)
                            break;
                    }
                }

                if (!pick)
                    break;
            }
            g_ptr = &floor_ptr->grid_array[y][x];

            /* Clear possible garbage of hidden trap */
            g_ptr->mimic = 0;

            /* Clear previous contents, add stairs */
            g_ptr->feat = (i < shaft_num) ? feat_state(owner_ptr, feat, FF_SHAFT) : feat;

            /* No longer "FLOOR" */
            g_ptr->info &= ~(CAVE_FLOOR);

            /* Success */
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
static void alloc_object(player_type *owner_ptr, int set, EFFECT_ID typ, int num)
{
    POSITION y = 0, x = 0;
    int k;
    int dummy = 0;
    grid_type *g_ptr;

    /* A small level has few objects. */
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    num = num * floor_ptr->height * floor_ptr->width / (MAX_HGT * MAX_WID) + 1;

    /* Place some objects */
    for (k = 0; k < num; k++) {
        /* Pick a "legal" spot */
        while (dummy < SAFE_MAX_ATTEMPTS) {
            bool room;

            dummy++;

            y = randint0(floor_ptr->height);
            x = randint0(floor_ptr->width);

            g_ptr = &floor_ptr->grid_array[y][x];

            /* Require "naked" floor grid */
            if (!is_floor_grid(g_ptr) || g_ptr->o_idx || g_ptr->m_idx)
                continue;

            /* Avoid player location */
            if (player_bold(owner_ptr, y, x))
                continue;

            /* Check for "room" */
            room = (floor_ptr->grid_array[y][x].info & CAVE_ROOM) ? TRUE : FALSE;

            /* Require corridor? */
            if ((set == ALLOC_SET_CORR) && room)
                continue;

            /* Require room? */
            if ((set == ALLOC_SET_ROOM) && !room)
                continue;

            /* Accept it */
            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS) {
            msg_print_wizard(owner_ptr, CHEAT_DUNGEON, _("アイテムの配置に失敗しました。", "Failed to place object."));
            return;
        }

        /* Place something */
        switch (typ) {
        case ALLOC_TYP_RUBBLE: {
            place_rubble(floor_ptr, y, x);
            floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
            break;
        }

        case ALLOC_TYP_TRAP: {
            place_trap(owner_ptr, y, x);
            floor_ptr->grid_array[y][x].info &= ~(CAVE_FLOOR);
            break;
        }

        case ALLOC_TYP_GOLD: {
            place_gold(owner_ptr, y, x);
            break;
        }

        case ALLOC_TYP_OBJECT: {
            place_object(owner_ptr, y, x, 0L);
            break;
        }
        }
    }
}

/*!
 * @brief クエストに関わるモンスターの配置を行う / Place quest monsters
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 成功したならばTRUEを返す
 */
bool place_quest_monsters(player_type *creature_ptr)
{
    int i;

    /* Handle the quest monster placements */
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (i = 0; i < max_q_idx; i++) {
        monster_race *r_ptr;
        BIT_FLAGS mode;
        int j;

        if (quest[i].status != QUEST_STATUS_TAKEN || (quest[i].type != QUEST_TYPE_KILL_LEVEL && quest[i].type != QUEST_TYPE_RANDOM)
            || quest[i].level != floor_ptr->dun_level || creature_ptr->dungeon_idx != quest[i].dungeon || (quest[i].flags & QUEST_FLAG_PRESET)) {
            /* Ignore it */
            continue;
        }

        r_ptr = &r_info[quest[i].r_idx];

        /* Hack -- "unique" monsters must be "unique" */
        if ((r_ptr->flags1 & RF1_UNIQUE) && (r_ptr->cur_num >= r_ptr->max_num))
            continue;

        mode = (PM_NO_KAGE | PM_NO_PET);

        if (!(r_ptr->flags1 & RF1_FRIENDS))
            mode |= PM_ALLOW_GROUP;

        for (j = 0; j < (quest[i].max_num - quest[i].cur_num); j++) {
            int k;

            for (k = 0; k < SAFE_MAX_ATTEMPTS; k++) {
                POSITION x = 0, y = 0;
                int l;

                /* Find an empty grid */
                for (l = SAFE_MAX_ATTEMPTS; l > 0; l--) {
                    grid_type *g_ptr;
                    feature_type *f_ptr;

                    y = randint0(floor_ptr->height);
                    x = randint0(floor_ptr->width);

                    g_ptr = &floor_ptr->grid_array[y][x];
                    f_ptr = &f_info[g_ptr->feat];

                    if (!have_flag(f_ptr->flags, FF_MOVE) && !have_flag(f_ptr->flags, FF_CAN_FLY))
                        continue;
                    if (!monster_can_enter(creature_ptr, y, x, r_ptr, 0))
                        continue;
                    if (distance(y, x, creature_ptr->y, creature_ptr->x) < 10)
                        continue;
                    if (g_ptr->info & CAVE_ICKY)
                        continue;
                    else
                        break;
                }

                /* Failed to place */
                if (!l)
                    return FALSE;

                /* Try to place the monster */
                if (place_monster_aux(creature_ptr, 0, y, x, quest[i].r_idx, mode)) {
                    /* Success */
                    break;
                } else {
                    /* Failure - Try again */
                    continue;
                }
            }

            /* Failed to place */
            if (k == SAFE_MAX_ATTEMPTS)
                return FALSE;
        }
    }

    return TRUE;
}

/*!
 * @brief フロアに洞窟や湖を配置する / Generate various caverns and lakes
 * @details There were moved from cave_gen().
 * @return なし
 */
static void gen_caverns_and_lakes(dungeon_type *dungeon_ptr, player_type *owner_ptr)
{
    floor_type *floor_ptr = owner_ptr->current_floor_ptr;
    /* Possible "destroyed" level */
    if ((floor_ptr->dun_level > 30) && one_in_(DUN_DEST * 2) && (small_levels) && (dungeon_ptr->flags1 & DF1_DESTROY)) {
        dun_data->destroyed = TRUE;

        /* extra rubble around the place looks cool */
        build_lake(owner_ptr, one_in_(2) ? LAKE_T_CAVE : LAKE_T_EARTH_VAULT);
    }

    /* Make a lake some of the time */
    if (one_in_(LAKE_LEVEL) && !dun_data->empty_level && !dun_data->destroyed && (dungeon_ptr->flags1 & DF1_LAKE_MASK)) {
        int count = 0;
        if (dungeon_ptr->flags1 & DF1_LAKE_WATER)
            count += 3;
        if (dungeon_ptr->flags1 & DF1_LAKE_LAVA)
            count += 3;
        if (dungeon_ptr->flags1 & DF1_LAKE_RUBBLE)
            count += 3;
        if (dungeon_ptr->flags1 & DF1_LAKE_TREE)
            count += 3;

        if (dungeon_ptr->flags1 & DF1_LAKE_LAVA) {
            /* Lake of Lava */
            if ((floor_ptr->dun_level > 80) && (randint0(count) < 2))
                dun_data->laketype = LAKE_T_LAVA;
            count -= 2;

            /* Lake of Lava2 */
            if (!dun_data->laketype && (floor_ptr->dun_level > 80) && one_in_(count))
                dun_data->laketype = LAKE_T_FIRE_VAULT;
            count--;
        }

        if ((dungeon_ptr->flags1 & DF1_LAKE_WATER) && !dun_data->laketype) {
            /* Lake of Water */
            if ((floor_ptr->dun_level > 50) && randint0(count) < 2)
                dun_data->laketype = LAKE_T_WATER;
            count -= 2;

            /* Lake of Water2 */
            if (!dun_data->laketype && (floor_ptr->dun_level > 50) && one_in_(count))
                dun_data->laketype = LAKE_T_WATER_VAULT;
            count--;
        }

        if ((dungeon_ptr->flags1 & DF1_LAKE_RUBBLE) && !dun_data->laketype) {
            /* Lake of rubble */
            if ((floor_ptr->dun_level > 35) && (randint0(count) < 2))
                dun_data->laketype = LAKE_T_CAVE;
            count -= 2;

            /* Lake of rubble2 */
            if (!dun_data->laketype && (floor_ptr->dun_level > 35) && one_in_(count))
                dun_data->laketype = LAKE_T_EARTH_VAULT;
            count--;
        }

        /* Lake of tree */
        if ((floor_ptr->dun_level > 5) && (dungeon_ptr->flags1 & DF1_LAKE_TREE) && !dun_data->laketype)
            dun_data->laketype = LAKE_T_AIR_VAULT;

        if (dun_data->laketype) {
            msg_print_wizard(owner_ptr, CHEAT_DUNGEON, _("湖を生成します。", "Lake on the level."));
            build_lake(owner_ptr, dun_data->laketype);
        }
    }

    if ((floor_ptr->dun_level > DUN_CAVERN) && !dun_data->empty_level && (dungeon_ptr->flags1 & DF1_CAVERN) && !dun_data->laketype && !dun_data->destroyed
        && (randint1(1000) < floor_ptr->dun_level)) {
        dun_data->cavern = TRUE;

        /* make a large fractal floor_ptr->grid_array in the middle of the dungeon */

        msg_print_wizard(owner_ptr, CHEAT_DUNGEON, _("洞窟を生成。", "Cavern on level."));
        build_cavern(owner_ptr);
    }

    /* Hack -- No destroyed "quest" levels */
    if (quest_number(owner_ptr, floor_ptr->dun_level))
        dun_data->destroyed = FALSE;
}

static bool has_river_flag(dungeon_type *dungeon_ptr)
{
    return dungeon_ptr->flags1 & (DF1_WATER_RIVER | DF1_LAVA_RIVER | DF1_ACID_RIVER | DF1_POISONOUS_RIVER);
}

/*!
 * @brief ダンジョン生成のメインルーチン / Generate a new dungeon level
 * @details Note that "dun_body" adds about 4000 bytes of memory to the stack.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param why エラー原因メッセージを返す
 * @return ダンジョン生成が全て無事に成功したらTRUEを返す。
 */
static bool cave_gen(player_type *player_ptr, concptr *why)
{
    int i, k;
    POSITION y, x;
    dun_data_type dun_body;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    dungeon_type *dungeon_ptr = &d_info[floor_ptr->dungeon_idx];

    floor_ptr->lite_n = 0;
    floor_ptr->mon_lite_n = 0;
    floor_ptr->redraw_n = 0;
    floor_ptr->view_n = 0;

    /* Global data */
    dun_data = &dun_body;

    dun_data->destroyed = FALSE;
    dun_data->empty_level = FALSE;
    dun_data->cavern = FALSE;
    dun_data->laketype = 0;

    /* Fill the arrays of floors and walls in the good proportions */
    set_floor_and_wall(floor_ptr->dungeon_idx);
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), NULL);

    /* Randomize the dungeon creation values */
    dun_tun_rnd = rand_range(DUN_TUN_RND_MIN, DUN_TUN_RND_MAX);
    dun_tun_chg = rand_range(DUN_TUN_CHG_MIN, DUN_TUN_CHG_MAX);
    dun_tun_con = rand_range(DUN_TUN_CON_MIN, DUN_TUN_CON_MAX);
    dun_tun_pen = rand_range(DUN_TUN_PEN_MIN, DUN_TUN_PEN_MAX);
    dun_tun_jct = rand_range(DUN_TUN_JCT_MIN, DUN_TUN_JCT_MAX);

    /* Actual maximum number of rooms on this level */
    dun_data->row_rooms = floor_ptr->height / BLOCK_HGT;
    dun_data->col_rooms = floor_ptr->width / BLOCK_WID;

    /* Initialize the room table */
    for (y = 0; y < dun_data->row_rooms; y++) {
        for (x = 0; x < dun_data->col_rooms; x++) {
            dun_data->room_map[y][x] = FALSE;
        }
    }

    /* No rooms yet */
    dun_data->cent_n = 0;

    /* Empty arena levels */
    if (ironman_empty_levels || ((dungeon_ptr->flags1 & DF1_ARENA) && (empty_levels && one_in_(EMPTY_LEVEL)))) {
        dun_data->empty_level = TRUE;
        msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("アリーナレベルを生成。", "Arena level."));
    }

    if (dun_data->empty_level) {
        /* Start with floors */
        for (y = 0; y < floor_ptr->height; y++) {
            for (x = 0; x < floor_ptr->width; x++) {
                place_bold(player_ptr, y, x, GB_FLOOR);
            }
        }

        /* Special boundary walls -- Top and bottom */
        for (x = 0; x < floor_ptr->width; x++) {
            place_bold(player_ptr, 0, x, GB_EXTRA);
            place_bold(player_ptr, floor_ptr->height - 1, x, GB_EXTRA);
        }

        /* Special boundary walls -- Left and right */
        for (y = 1; y < (floor_ptr->height - 1); y++) {
            place_bold(player_ptr, y, 0, GB_EXTRA);
            place_bold(player_ptr, y, floor_ptr->width - 1, GB_EXTRA);
        }
    } else {
        /* Start with walls */
        for (y = 0; y < floor_ptr->height; y++) {
            for (x = 0; x < floor_ptr->width; x++) {
                place_bold(player_ptr, y, x, GB_EXTRA);
            }
        }
    }

    /* Generate various caverns and lakes */
    gen_caverns_and_lakes(dungeon_ptr, player_ptr);

    /* Build maze */
    if (dungeon_ptr->flags1 & DF1_MAZE) {
        build_maze_vault(player_ptr, floor_ptr->width / 2 - 1, floor_ptr->height / 2 - 1, floor_ptr->width - 4, floor_ptr->height - 4, FALSE);

        /* Place 3 or 4 down stairs near some walls */
        if (!alloc_stairs(player_ptr, feat_down_stair, rand_range(2, 3), 3)) {
            *why = _("迷宮ダンジョンの下り階段生成に失敗", "Failed to alloc up stairs in maze dungeon.");
            return FALSE;
        }

        /* Place 1 or 2 up stairs near some walls */
        if (!alloc_stairs(player_ptr, feat_up_stair, 1, 3)) {
            *why = _("迷宮ダンジョンの上り階段生成に失敗", "Failed to alloc down stairs in maze dungeon.");
            return FALSE;
        }
    }

    /* Build some rooms */
    else {
        int tunnel_fail_count = 0;

        /*
         * Build each type of room in turn until we cannot build any more.
         */
        if (!generate_rooms(player_ptr)) {
            *why = _("部屋群の生成に失敗", "Failed to generate rooms");
            return FALSE;
        }

        /* Make a hole in the dungeon roof sometimes at level 1 */
        if (floor_ptr->dun_level == 1) {
            while (one_in_(DUN_MOS_DEN)) {
                place_trees(player_ptr, randint1(floor_ptr->width - 2), randint1(floor_ptr->height - 2));
            }
        }

        if (dun_data->destroyed) {
            destroy_level(player_ptr);
        }

        if (has_river_flag(dungeon_ptr) && one_in_(3) && (randint1(floor_ptr->dun_level) > 5)) {
            add_river(floor_ptr);
        }

        /* Hack -- Scramble the room order */
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

        /* Start with no tunnel doors */
        dun_data->door_n = 0;

        /* Hack -- connect the first room to the last room */
        y = dun_data->cent[dun_data->cent_n - 1].y;
        x = dun_data->cent[dun_data->cent_n - 1].x;

        /* Connect all the rooms together */
        for (i = 0; i < dun_data->cent_n; i++) {
            int j;

            /* Reset the arrays */
            dun_data->tunn_n = 0;
            dun_data->wall_n = 0;

            /* Connect the room to the previous room */
            if (randint1(floor_ptr->dun_level) > dungeon_ptr->tunnel_percent) {
                /* make cavelike tunnel */
                (void)build_tunnel2(player_ptr, dun_data->cent[i].x, dun_data->cent[i].y, x, y, 2, 2);
            } else {
                /* make normal tunnel */
                if (!build_tunnel(player_ptr, dun_data->cent[i].y, dun_data->cent[i].x, y, x))
                    tunnel_fail_count++;
            }

            if (tunnel_fail_count >= 2) {
                *why = _("トンネル接続に失敗", "Failed to generate tunnels");
                return FALSE;
            }

            /* Turn the tunnel into corridor */
            for (j = 0; j < dun_data->tunn_n; j++) {
                grid_type *g_ptr;
                feature_type *f_ptr;
                y = dun_data->tunn[j].y;
                x = dun_data->tunn[j].x;
                g_ptr = &floor_ptr->grid_array[y][x];
                f_ptr = &f_info[g_ptr->feat];

                /* Clear previous contents (if not a lake), add a floor */
                if (!have_flag(f_ptr->flags, FF_MOVE) || (!have_flag(f_ptr->flags, FF_WATER) && !have_flag(f_ptr->flags, FF_LAVA))) {
                    /* Clear mimic type */
                    g_ptr->mimic = 0;

                    place_grid(player_ptr, g_ptr, GB_FLOOR);
                }
            }

            /* Apply the piercings that we found */
            for (j = 0; j < dun_data->wall_n; j++) {
                grid_type *g_ptr;
                y = dun_data->wall[j].y;
                x = dun_data->wall[j].x;
                g_ptr = &floor_ptr->grid_array[y][x];

                /* Clear mimic type */
                g_ptr->mimic = 0;

                /* Clear previous contents, add up floor */
                place_grid(player_ptr, g_ptr, GB_FLOOR);

                /* Occasional doorway */
                if ((randint0(100) < dun_tun_pen) && !(dungeon_ptr->flags1 & DF1_NO_DOORS)) {
                    /* Place a random door */
                    place_random_door(player_ptr, y, x, TRUE);
                }
            }

            /* Remember the "previous" room */
            y = dun_data->cent[i].y;
            x = dun_data->cent[i].x;
        }

        /* Place intersection doors */
        for (i = 0; i < dun_data->door_n; i++) {
            /* Extract junction location */
            y = dun_data->door[i].y;
            x = dun_data->door[i].x;

            /* Try placing doors */
            try_door(player_ptr, y, x - 1);
            try_door(player_ptr, y, x + 1);
            try_door(player_ptr, y - 1, x);
            try_door(player_ptr, y + 1, x);
        }

        /* Place 3 or 4 down stairs near some walls */
        if (!alloc_stairs(player_ptr, feat_down_stair, rand_range(3, 4), 3)) {
            *why = _("下り階段生成に失敗", "Failed to generate down stairs.");
            return FALSE;
        }

        /* Place 1 or 2 up stairs near some walls */
        if (!alloc_stairs(player_ptr, feat_up_stair, rand_range(1, 2), 3)) {
            *why = _("上り階段生成に失敗", "Failed to generate up stairs.");
            return FALSE;
        }
    }

    if (!dun_data->laketype) {
        if (dungeon_ptr->stream2) {
            /* Hack -- Add some quartz streamers */
            for (i = 0; i < DUN_STR_QUA; i++) {
                build_streamer(player_ptr, dungeon_ptr->stream2, DUN_STR_QC);
            }
        }

        if (dungeon_ptr->stream1) {
            /* Hack -- Add some magma streamers */
            for (i = 0; i < DUN_STR_MAG; i++) {
                build_streamer(player_ptr, dungeon_ptr->stream1, DUN_STR_MC);
            }
        }
    }

    /* Special boundary walls -- Top and bottom */
    for (x = 0; x < floor_ptr->width; x++) {
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[0][x]);
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[floor_ptr->height - 1][x]);
    }

    /* Special boundary walls -- Left and right */
    for (y = 1; y < (floor_ptr->height - 1); y++) {
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[y][0]);
        place_bound_perm_wall(player_ptr, &floor_ptr->grid_array[y][floor_ptr->width - 1]);
    }

    /* Determine the character location */
    if (!new_player_spot(player_ptr)) {
        *why = _("プレイヤー配置に失敗", "Failed to place a player");
        return FALSE;
    }

    if (!place_quest_monsters(player_ptr)) {
        *why = _("クエストモンスター配置に失敗", "Failed to place a quest monster");
        return FALSE;
    }

    /* Basic "amount" */
    k = (floor_ptr->dun_level / 3);
    if (k > 10)
        k = 10;
    if (k < 2)
        k = 2;

    /* Pick a base number of monsters */
    i = dungeon_ptr->min_m_alloc_level;

    /* To make small levels a bit more playable */
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

    /* Put some monsters in the dungeon */
    for (i = i + k; i > 0; i--) {
        (void)alloc_monster(player_ptr, 0, PM_ALLOW_SLEEP, summon_specific);
    }

    /* Place some traps in the dungeon */
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

    /* Put some rubble in corridors (except NO_CAVE dungeon (Castle)) */
    if (!(dungeon_ptr->flags1 & DF1_NO_CAVE))
        alloc_object(player_ptr, ALLOC_SET_CORR, ALLOC_TYP_RUBBLE, randint1(k));

    /* Mega Hack -- No object at first level of deeper dungeon */
    if (player_ptr->enter_dungeon && floor_ptr->dun_level > 1) {
        /* No stair scum! */
        floor_ptr->object_level = 1;
    }

    /* Put some objects in rooms */
    alloc_object(player_ptr, ALLOC_SET_ROOM, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ROOM, 3));

    /* Put some objects/gold in the dungeon */
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_OBJECT, randnor(DUN_AMT_ITEM, 3));
    alloc_object(player_ptr, ALLOC_SET_BOTH, ALLOC_TYP_GOLD, randnor(DUN_AMT_GOLD, 3));

    /* Set back to default */
    floor_ptr->object_level = floor_ptr->base_level;

    /* Put the Guardian */
    if (!alloc_guardian(player_ptr, TRUE)) {
        *why = _("ダンジョンの主配置に失敗", "Failed to place a dungeon guardian");
        return FALSE;
    }

    bool is_empty_or_dark = dun_data->empty_level;
    is_empty_or_dark &= !one_in_(DARK_EMPTY) || (randint1(100) > floor_ptr->dun_level);
    is_empty_or_dark &= (dungeon_ptr->flags1 & DF1_DARKNESS) == 0;
    if (!is_empty_or_dark)
        return TRUE;

    /* Lite the floor_ptr->grid_array */
    for (y = 0; y < floor_ptr->height; y++) {
        for (x = 0; x < floor_ptr->width; x++) {
            floor_ptr->grid_array[y][x].info |= (CAVE_GLOW);
        }
    }

    return TRUE;
}

/*!
 * @brief 闘技場用のアリーナ地形を作成する / Builds the arena after it is entered -KMW-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void build_arena(player_type *player_ptr, POSITION *start_y, POSITION *start_x)
{
    POSITION yval, y_height, y_depth, xval, x_left, x_right;
    POSITION i, j;

    yval = SCREEN_HGT / 2;
    xval = SCREEN_WID / 2;
    y_height = yval - 10;
    y_depth = yval + 10;
    x_left = xval - 32;
    x_right = xval + 32;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (i = y_height; i <= y_height + 5; i++)
        for (j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    for (i = y_depth; i >= y_depth - 5; i--)
        for (j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    for (j = x_left; j <= x_left + 17; j++)
        for (i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    for (j = x_right; j >= x_right - 17; j--)
        for (i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    place_bold(player_ptr, y_height + 6, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_left + 18].info |= (CAVE_GLOW | CAVE_MARK);
    place_bold(player_ptr, y_depth - 6, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 6][x_left + 18].info |= (CAVE_GLOW | CAVE_MARK);
    place_bold(player_ptr, y_height + 6, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_right - 18].info |= (CAVE_GLOW | CAVE_MARK);
    place_bold(player_ptr, y_depth - 6, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 6][x_right - 18].info |= (CAVE_GLOW | CAVE_MARK);

    *start_y = y_height + 5;
    *start_x = xval;
    floor_ptr->grid_array[*start_y][*start_x].feat = f_tag_to_index("ARENA_GATE");
    floor_ptr->grid_array[*start_y][*start_x].info |= (CAVE_GLOW | CAVE_MARK);
}

/*!
 * @brief 挑戦時闘技場への入場処理 / Town logic flow for generation of arena -KMW-
 * @return なし
 */
static void generate_challenge_arena(player_type *challanger_ptr)
{
    POSITION y, x;
    POSITION qy = 0;
    POSITION qx = 0;

    /* Smallest area */
    floor_type *floor_ptr = challanger_ptr->current_floor_ptr;
    floor_ptr->height = SCREEN_HGT;
    floor_ptr->width = SCREEN_WID;

    /* Start with solid walls */
    for (y = 0; y < MAX_HGT; y++) {
        for (x = 0; x < MAX_WID; x++) {
            /* Create "solid" perma-wall */
            place_bold(challanger_ptr, y, x, GB_SOLID_PERM);

            /* Illuminate and memorize the walls */
            floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    /* Then place some floors */
    for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++) {
        for (x = qx + 1; x < qx + SCREEN_WID - 1; x++) {
            /* Create empty floor */
            floor_ptr->grid_array[y][x].feat = feat_floor;
        }
    }

    build_arena(challanger_ptr, &y, &x);
    player_place(challanger_ptr, y, x);

    if (!place_monster_aux(
            challanger_ptr, 0, challanger_ptr->y + 5, challanger_ptr->x, arena_info[challanger_ptr->arena_number].r_idx, (PM_NO_KAGE | PM_NO_PET))) {
        challanger_ptr->exit_bldg = TRUE;
        challanger_ptr->arena_number++;
        msg_print(_("相手は欠場した。あなたの不戦勝だ。", "The enemy is unable appear. You won by default."));
    }
}

/*!
 * @brief モンスター闘技場のフロア生成 / Builds the arena after it is entered -KMW-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void build_battle(player_type *player_ptr, POSITION *y, POSITION *x)
{
    POSITION yval, y_height, y_depth, xval, x_left, x_right;
    register int i, j;

    yval = SCREEN_HGT / 2;
    xval = SCREEN_WID / 2;
    y_height = yval - 10;
    y_depth = yval + 10;
    x_left = xval - 32;
    x_right = xval + 32;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (i = y_height; i <= y_height + 5; i++)
        for (j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    for (i = y_depth; i >= y_depth - 3; i--)
        for (j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    for (j = x_left; j <= x_left + 17; j++)
        for (i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    for (j = x_right; j >= x_right - 17; j--)
        for (i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    place_bold(player_ptr, y_height + 6, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_left + 18].info |= (CAVE_GLOW | CAVE_MARK);
    place_bold(player_ptr, y_depth - 4, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 4][x_left + 18].info |= (CAVE_GLOW | CAVE_MARK);
    place_bold(player_ptr, y_height + 6, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_right - 18].info |= (CAVE_GLOW | CAVE_MARK);
    place_bold(player_ptr, y_depth - 4, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 4][x_right - 18].info |= (CAVE_GLOW | CAVE_MARK);

    for (i = y_height + 1; i <= y_height + 5; i++)
        for (j = x_left + 20 + 2 * (y_height + 5 - i); j <= x_right - 20 - 2 * (y_height + 5 - i); j++) {
            floor_ptr->grid_array[i][j].feat = feat_permanent_glass_wall;
        }

    i = y_height + 1;
    j = xval;
    floor_ptr->grid_array[i][j].feat = f_tag_to_index("BUILDING_3");
    floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);

    *y = i;
    *x = j;
}

/*!
 * @brief モンスター闘技場への導入処理 / Town logic flow for generation of arena -KMW-
 * @return なし
 */
static void generate_gambling_arena(player_type *creature_ptr)
{
    POSITION y, x;
    MONSTER_IDX i;
    POSITION qy = 0;
    POSITION qx = 0;

    /* Start with solid walls */
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (y = 0; y < MAX_HGT; y++) {
        for (x = 0; x < MAX_WID; x++) {
            /* Create "solid" perma-wall */
            place_bold(creature_ptr, y, x, GB_SOLID_PERM);

            /* Illuminate and memorize the walls */
            floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    /* Then place some floors */
    for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++) {
        for (x = qx + 1; x < qx + SCREEN_WID - 1; x++) {
            /* Create empty floor */
            floor_ptr->grid_array[y][x].feat = feat_floor;
        }
    }

    build_battle(creature_ptr, &y, &x);

    player_place(creature_ptr, y, x);

    for (i = 0; i < 4; i++) {
        place_monster_aux(creature_ptr, 0, creature_ptr->y + 8 + (i / 2) * 4, creature_ptr->x - 2 + (i % 2) * 4, battle_mon[i], (PM_NO_KAGE | PM_NO_PET));
        set_friendly(&floor_ptr->m_list[floor_ptr->grid_array[creature_ptr->y + 8 + (i / 2) * 4][creature_ptr->x - 2 + (i % 2) * 4].m_idx]);
    }
    for (i = 1; i < floor_ptr->m_max; i++) {
        monster_type *m_ptr = &floor_ptr->m_list[i];

        if (!monster_is_valid(m_ptr))
            continue;

        m_ptr->mflag2 |= (MFLAG2_MARK | MFLAG2_SHOW);
        update_monster(creature_ptr, i, FALSE);
    }
}

/*!
 * @brief 固定マップクエストのフロア生成 / Generate a quest level
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void generate_fixed_floor(player_type *player_ptr)
{
    POSITION x, y;

    /* Start with perm walls */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (y = 0; y < floor_ptr->height; y++) {
        for (x = 0; x < floor_ptr->width; x++) {
            place_bold(player_ptr, y, x, GB_SOLID_PERM);
        }
    }

    /* Set the quest level */
    floor_ptr->base_level = quest[floor_ptr->inside_quest].level;
    floor_ptr->dun_level = floor_ptr->base_level;
    floor_ptr->object_level = floor_ptr->base_level;
    floor_ptr->monster_level = floor_ptr->base_level;

    if (record_stair)
        exe_write_diary(player_ptr, DIARY_TO_QUEST, floor_ptr->inside_quest, NULL);
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), NULL);

    init_flags = INIT_CREATE_DUNGEON;

    parse_fixed_map(player_ptr, "q_info.txt", 0, 0, MAX_HGT, MAX_WID);
}

/*!
 * @brief ダンジョン時のランダムフロア生成 / Make a real level
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param concptr
 * @return フロアの生成に成功したらTRUE
 */
static bool level_gen(player_type *player_ptr, concptr *why)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    DUNGEON_IDX d_idx = floor_ptr->dungeon_idx;

    if ((always_small_levels || ironman_small_levels || (one_in_(SMALL_LEVEL) && small_levels) || (d_info[d_idx].flags1 & DF1_BEGINNER)
            || (d_info[d_idx].flags1 & DF1_SMALLEST))
        && !(d_info[d_idx].flags1 & DF1_BIG)) {
        int level_height, level_width;
        if (d_info[d_idx].flags1 & DF1_SMALLEST) {
            level_height = 1;
            level_width = 1;
        } else if (d_info[d_idx].flags1 & DF1_BEGINNER) {
            level_height = 2;
            level_width = 2;
        } else {
            level_height = randint1(MAX_HGT / SCREEN_HGT);
            level_width = randint1(MAX_WID / SCREEN_WID);
            bool is_first_level_area = TRUE;
            bool is_max_area = (level_height == MAX_HGT / SCREEN_HGT) && (level_width == MAX_WID / SCREEN_WID);
            while (is_first_level_area || is_max_area) {
                level_height = randint1(MAX_HGT / SCREEN_HGT);
                level_width = randint1(MAX_WID / SCREEN_WID);
                is_first_level_area = FALSE;
                is_max_area = (level_height == MAX_HGT / SCREEN_HGT) && (level_width == MAX_WID / SCREEN_WID);
            }
        }

        floor_ptr->height = level_height * SCREEN_HGT;
        floor_ptr->width = level_width * SCREEN_WID;

        /* Assume illegal panel */
        panel_row_min = floor_ptr->height;
        panel_col_min = floor_ptr->width;

        msg_format_wizard(
            player_ptr, CHEAT_DUNGEON, _("小さなフロア: X:%d, Y:%d", "A 'small' dungeon level: X:%d, Y:%d."), floor_ptr->width, floor_ptr->height);
    } else {
        /* Big dungeon */
        floor_ptr->height = MAX_HGT;
        floor_ptr->width = MAX_WID;

        /* Assume illegal panel */
        panel_row_min = floor_ptr->height;
        panel_col_min = floor_ptr->width;
    }

    if (!cave_gen(player_ptr, why)) {
        return FALSE;
    }

    else
        return TRUE;
}

/*!
 * @brief フロアに存在する全マスの記憶状態を初期化する / Wipe all unnecessary flags after grid_array generation
 * @return なし
 */
void wipe_generate_random_floor_flags(floor_type *floor_ptr)
{
    POSITION x, y;

    for (y = 0; y < floor_ptr->height; y++) {
        for (x = 0; x < floor_ptr->width; x++) {
            /* Wipe unused flags */
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
        }
    }

    if (floor_ptr->dun_level) {
        for (y = 1; y < floor_ptr->height - 1; y++) {
            for (x = 1; x < floor_ptr->width - 1; x++) {
                /* There might be trap */
                floor_ptr->grid_array[y][x].info |= CAVE_UNSAFE;
            }
        }
    }
}

/*!
 * @brief フロアの全情報を初期化する / Clear and empty floor.
 * @parama player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void clear_cave(player_type *player_ptr)
{
    POSITION x, y;
    int i;

    /* Very simplified version of wipe_o_list() */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    (void)C_WIPE(floor_ptr->o_list, floor_ptr->o_max, object_type);
    floor_ptr->o_max = 1;
    floor_ptr->o_cnt = 0;

    /* Very simplified version of wipe_m_list() */
    for (i = 1; i < max_r_idx; i++)
        r_info[i].cur_num = 0;
    (void)C_WIPE(floor_ptr->m_list, floor_ptr->m_max, monster_type);
    floor_ptr->m_max = 1;
    floor_ptr->m_cnt = 0;
    for (i = 0; i < MAX_MTIMED; i++)
        floor_ptr->mproc_max[i] = 0;

    /* Pre-calc cur_num of pets in party_mon[] */
    precalc_cur_num_of_pet(player_ptr);

    /* Start with a blank floor_ptr->grid_array */
    for (y = 0; y < MAX_HGT; y++) {
        for (x = 0; x < MAX_WID; x++) {
            grid_type *g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info = 0;
            g_ptr->feat = 0;
            g_ptr->o_idx = 0;
            g_ptr->m_idx = 0;
            g_ptr->special = 0;
            g_ptr->mimic = 0;
            g_ptr->cost = 0;
            g_ptr->dist = 0;
            g_ptr->when = 0;
        }
    }

    /* Set the base level */
    floor_ptr->base_level = floor_ptr->dun_level;

    /* Reset the monster generation level */
    floor_ptr->monster_level = floor_ptr->base_level;

    /* Reset the object generation level */
    floor_ptr->object_level = floor_ptr->base_level;
}

/*!
 * ダンジョンのランダムフロアを生成する / Generates a random dungeon level -RAK-
 * @parama player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @note Hack -- regenerate any "overflow" levels
 */
void generate_floor(player_type *player_ptr)
{
    int num;
    /* Fill the arrays of floors and walls in the good proportions */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->dungeon_idx = player_ptr->dungeon_idx;
    set_floor_and_wall(floor_ptr->dungeon_idx);

    /* Generate */
    for (num = 0; TRUE; num++) {
        bool okay = TRUE;
        concptr why = NULL;

        clear_cave(player_ptr);

        /* Mega-Hack -- no player yet */
        player_ptr->x = player_ptr->y = 0;

        if (floor_ptr->inside_arena) {
            generate_challenge_arena(player_ptr);
        }

        else if (player_ptr->phase_out) {
            generate_gambling_arena(player_ptr);
        }

        else if (floor_ptr->inside_quest) {
            generate_fixed_floor(player_ptr);
        }

        /* Build the town */
        else if (!floor_ptr->dun_level) {
            /* Make the wilderness */
            if (player_ptr->wild_mode)
                wilderness_gen_small(player_ptr);
            else
                wilderness_gen(player_ptr);
        }

        /* Build a real level */
        else {
            okay = level_gen(player_ptr, &why);
        }

        /* Prevent object over-flow */
        if (floor_ptr->o_max >= current_world_ptr->max_o_idx) {
            why = _("アイテムが多すぎる", "too many objects");
            okay = FALSE;
        }
        /* Prevent monster over-flow */
        else if (floor_ptr->m_max >= current_world_ptr->max_m_idx) {
            why = _("モンスターが多すぎる", "too many monsters");
            okay = FALSE;
        }

        /* Accept */
        if (okay)
            break;

        if (why)
            msg_format(_("生成やり直し(%s)", "Generation restarted (%s)"), why);

        wipe_o_list(floor_ptr);
        wipe_monsters_list(player_ptr);
    }

    /* Glow deep lava and building entrances tempor */
    glow_deep_lava_and_bldg(player_ptr);

    /* Reset flag */
    player_ptr->enter_dungeon = FALSE;

    wipe_generate_random_floor_flags(floor_ptr);
}

/*!
 * @brief build_tunnel用に通路を掘るための方向をランダムに決める / Pick a random direction
 * @param rdir Y方向に取るべきベクトル値を返す参照ポインタ
 * @param cdir X方向に取るべきベクトル値を返す参照ポインタ
 * @return なし
 */
static void rand_dir(POSITION *rdir, POSITION *cdir)
{
    /* Pick a random direction */
    int i = randint0(4);

    /* Extract the dy/dx components */
    *rdir = ddy_ddd[i];
    *cdir = ddx_ddd[i];
}

/*!
 * @brief build_tunnel用に通路を掘るための方向を位置関係通りに決める / Always picks a correct direction
 * @param rdir Y方向に取るべきベクトル値を返す参照ポインタ
 * @param cdir X方向に取るべきベクトル値を返す参照ポインタ
 * @param y1 始点Y座標
 * @param x1 始点X座標
 * @param y2 終点Y座標
 * @param x2 終点X座標
 * @return なし
 */
static void correct_dir(POSITION *rdir, POSITION *cdir, POSITION y1, POSITION x1, POSITION y2, POSITION x2)
{
    /* Extract vertical and horizontal directions */
    *rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
    *cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;

    /* Never move diagonally */
    if (*rdir && *cdir) {
        if (randint0(100) < 50)
            *rdir = 0;
        else
            *cdir = 0;
    }
}

/*!
 * @brief 部屋間のトンネルを生成する / Constructs a tunnel between two points
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param row1 始点Y座標
 * @param col1 始点X座標
 * @param row2 終点Y座標
 * @param col2 終点X座標
 * @return 生成に成功したらTRUEを返す
 * @details
 * This function must be called BEFORE any streamers are created,\n
 * since we use the special "granite wall" sub-types to keep track\n
 * of legal places for corridors to pierce rooms.\n
 *\n
 * We use "door_flag" to prevent excessive construction of doors\n
 * along overlapping corridors.\n
 *\n
 * We queue the tunnel grids to prevent door creation along a corridor\n
 * which intersects itself.\n
 *\n
 * We queue the wall piercing grids to prevent a corridor from leaving\n
 * a room and then coming back in through the same entrance.\n
 *\n
 * We "pierce" grids which are "outer" walls of rooms, and when we\n
 * do so, we change all adjacent "outer" walls of rooms into "solid"\n
 * walls so that no two corridors may use adjacent grids for exits.\n
 *\n
 * The "solid" wall check prevents corridors from "chopping" the\n
 * corners of rooms off, as well as "silly" door placement, and\n
 * "excessively wide" room entrances.\n
 *\n
 * Kind of walls:\n
 *   extra -- walls\n
 *   inner -- inner room walls\n
 *   outer -- outer room walls\n
 *   solid -- solid room walls\n
 */
bool build_tunnel(player_type *player_ptr, POSITION row1, POSITION col1, POSITION row2, POSITION col2)
{
    POSITION y, x;
    POSITION tmp_row, tmp_col;
    POSITION row_dir, col_dir;
    POSITION start_row, start_col;
    int main_loop_count = 0;

    bool door_flag = FALSE;

    grid_type *g_ptr;

    /* Save the starting location */
    start_row = row1;
    start_col = col1;

    /* Start out in the correct direction */
    correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

    /* Keep going until done (or bored) */
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    while ((row1 != row2) || (col1 != col2)) {
        /* Mega-Hack -- Paranoia -- prevent infinite loops */
        if (main_loop_count++ > 2000)
            return FALSE;

        /* Allow bends in the tunnel */
        if (randint0(100) < dun_tun_chg) {
            /* Acquire the correct direction */
            correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

            /* Random direction */
            if (randint0(100) < dun_tun_rnd) {
                rand_dir(&row_dir, &col_dir);
            }
        }

        /* Get the next location */
        tmp_row = row1 + row_dir;
        tmp_col = col1 + col_dir;

        /* Extremely Important -- do not leave the dungeon */
        while (!in_bounds(floor_ptr, tmp_row, tmp_col)) {
            /* Acquire the correct direction */
            correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);

            /* Random direction */
            if (randint0(100) < dun_tun_rnd) {
                rand_dir(&row_dir, &col_dir);
            }

            /* Get the next location */
            tmp_row = row1 + row_dir;
            tmp_col = col1 + col_dir;
        }

        g_ptr = &floor_ptr->grid_array[tmp_row][tmp_col];

        /* Avoid "solid" walls */
        if (is_solid_grid(g_ptr))
            continue;

        /* Pierce "outer" walls of rooms */
        if (is_outer_grid(g_ptr)) {
            /* Acquire the "next" location */
            y = tmp_row + row_dir;
            x = tmp_col + col_dir;

            /* Hack -- Avoid outer/solid walls */
            if (is_outer_bold(floor_ptr, y, x))
                continue;
            if (is_solid_bold(floor_ptr, y, x))
                continue;

            /* Accept this location */
            row1 = tmp_row;
            col1 = tmp_col;

            /* Save the wall location */
            if (dun_data->wall_n < WALL_MAX) {
                dun_data->wall[dun_data->wall_n].y = row1;
                dun_data->wall[dun_data->wall_n].x = col1;
                dun_data->wall_n++;
            } else
                return FALSE;

            /* Forbid re-entry near this piercing */
            for (y = row1 - 1; y <= row1 + 1; y++) {
                for (x = col1 - 1; x <= col1 + 1; x++) {
                    /* Convert adjacent "outer" walls as "solid" walls */
                    if (is_outer_bold(floor_ptr, y, x)) {
                        /* Change the wall to a "solid" wall */
                        place_bold(player_ptr, y, x, GB_SOLID_NOPERM);
                    }
                }
            }
        }

        /* Travel quickly through rooms */
        else if (g_ptr->info & (CAVE_ROOM)) {
            /* Accept the location */
            row1 = tmp_row;
            col1 = tmp_col;
        }

        /* Tunnel through all other walls */
        else if (is_extra_grid(g_ptr) || is_inner_grid(g_ptr) || is_solid_grid(g_ptr)) {
            /* Accept this location */
            row1 = tmp_row;
            col1 = tmp_col;

            /* Save the tunnel location */
            if (dun_data->tunn_n < TUNN_MAX) {
                dun_data->tunn[dun_data->tunn_n].y = row1;
                dun_data->tunn[dun_data->tunn_n].x = col1;
                dun_data->tunn_n++;
            } else
                return FALSE;

            /* Allow door in next grid */
            door_flag = FALSE;
        }

        /* Handle corridor intersections or overlaps */
        else {
            /* Accept the location */
            row1 = tmp_row;
            col1 = tmp_col;

            /* Collect legal door locations */
            if (!door_flag) {
                /* Save the door location */
                if (dun_data->door_n < DOOR_MAX) {
                    dun_data->door[dun_data->door_n].y = row1;
                    dun_data->door[dun_data->door_n].x = col1;
                    dun_data->door_n++;
                } else
                    return FALSE;

                /* No door in next grid */
                door_flag = TRUE;
            }

            /* Hack -- allow pre-emptive tunnel termination */
            if (randint0(100) >= dun_tun_con) {
                /* Distance between row1 and start_row */
                tmp_row = row1 - start_row;
                if (tmp_row < 0)
                    tmp_row = (-tmp_row);

                /* Distance between col1 and start_col */
                tmp_col = col1 - start_col;
                if (tmp_col < 0)
                    tmp_col = (-tmp_col);

                /* Terminate the tunnel */
                if ((tmp_row > 10) || (tmp_col > 10))
                    break;
            }
        }
    }

    return TRUE;
}

/*!
 * @brief トンネル生成のための基準点を指定する。
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param x 基準点を指定するX座標の参照ポインタ、適時値が修正される。
 * @param y 基準点を指定するY座標の参照ポインタ、適時値が修正される。
 * @param affectwall (調査中)
 * @return なし
 * @details
 * This routine adds the square to the tunnel\n
 * It also checks for SOLID walls - and returns a nearby\n
 * non-SOLID square in (x,y) so that a simple avoiding\n
 * routine can be used. The returned boolean value reflects\n
 * whether or not this routine hit a SOLID wall.\n
 *\n
 * "affectwall" toggles whether or not this new square affects\n
 * the boundaries of rooms. - This is used by the catacomb\n
 * routine.\n
 * @todo 特に詳細な処理の意味を調査すべし
 */
static bool set_tunnel(player_type *player_ptr, POSITION *x, POSITION *y, bool affectwall)
{
    int i, j, dx, dy;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[*y][*x];

    if (!in_bounds(floor_ptr, *y, *x))
        return TRUE;

    if (is_inner_grid(g_ptr)) {
        return TRUE;
    }

    if (is_extra_bold(floor_ptr, *y, *x)) {
        /* Save the tunnel location */
        if (dun_data->tunn_n < TUNN_MAX) {
            dun_data->tunn[dun_data->tunn_n].y = *y;
            dun_data->tunn[dun_data->tunn_n].x = *x;
            dun_data->tunn_n++;

            return TRUE;
        } else
            return FALSE;
    }

    if (is_floor_bold(floor_ptr, *y, *x)) {
        /* Don't do anything */
        return TRUE;
    }

    if (is_outer_grid(g_ptr) && affectwall) {
        /* Save the wall location */
        if (dun_data->wall_n < WALL_MAX) {
            dun_data->wall[dun_data->wall_n].y = *y;
            dun_data->wall[dun_data->wall_n].x = *x;
            dun_data->wall_n++;
        } else
            return FALSE;

        /* Forbid re-entry near this piercing */
        for (j = *y - 1; j <= *y + 1; j++) {
            for (i = *x - 1; i <= *x + 1; i++) {
                /* Convert adjacent "outer" walls as "solid" walls */
                if (is_outer_bold(floor_ptr, j, i)) {
                    /* Change the wall to a "solid" wall */
                    place_bold(player_ptr, j, i, GB_SOLID_NOPERM);
                }
            }
        }

        /* Clear mimic type */
        floor_ptr->grid_array[*y][*x].mimic = 0;

        place_bold(player_ptr, *y, *x, GB_FLOOR);

        return TRUE;
    }

    if (is_solid_grid(g_ptr) && affectwall) {
        /* cannot place tunnel here - use a square to the side */

        /* find usable square and return value in (x,y) */

        i = 50;

        dy = 0;
        dx = 0;
        while ((i > 0) && is_solid_bold(floor_ptr, *y + dy, *x + dx)) {
            dy = randint0(3) - 1;
            dx = randint0(3) - 1;

            if (!in_bounds(floor_ptr, *y + dy, *x + dx)) {
                dx = 0;
                dy = 0;
            }

            i--;
        }

        if (i == 0) {
            /* Failed for some reason: hack - ignore the solidness */
            place_grid(player_ptr, g_ptr, GB_OUTER);
            dx = 0;
            dy = 0;
        }

        /* Give new, acceptable coordinate. */
        *x = *x + dx;
        *y = *y + dy;

        return FALSE;
    }

    return TRUE;
}

/*!
 * @brief 外壁を削って「カタコンベ状」の通路を作成する / This routine creates the catacomb-like tunnels by removing extra rock.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param x 基準点のX座標
 * @param y 基準点のY座標
 * @return なし
 * @details
 * Note that this routine is only called on "even" squares - so it gives
 * a natural checkerboard pattern.
 */
static void create_cata_tunnel(player_type *player_ptr, POSITION x, POSITION y)
{
    POSITION x1, y1;

    /* Build tunnel */
    x1 = x - 1;
    y1 = y;
    set_tunnel(player_ptr, &x1, &y1, FALSE);

    x1 = x + 1;
    y1 = y;
    set_tunnel(player_ptr, &x1, &y1, FALSE);

    x1 = x;
    y1 = y - 1;
    set_tunnel(player_ptr, &x1, &y1, FALSE);

    x1 = x;
    y1 = y + 1;
    set_tunnel(player_ptr, &x1, &y1, FALSE);
}

/*!
 * @brief トンネル生成処理（詳細調査中）/ This routine does the bulk of the work in creating the new types of tunnels.
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @todo 詳細用調査
 * @details
 * It is designed to use very simple algorithms to go from (x1,y1) to (x2,y2)\n
 * It doesn't need to add any complexity - straight lines are fine.\n
 * The SOLID walls are avoided by a recursive algorithm which tries random ways\n
 * around the obstical until it works.  The number of itterations is counted, and it\n
 * this gets too large the routine exits. This should stop any crashes - but may leave\n
 * small gaps in the tunnel where there are too many SOLID walls.\n
 *\n
 * Type 1 tunnels are extremely simple - straight line from A to B.  This is only used\n
 * as a part of the dodge SOLID walls algorithm.\n
 *\n
 * Type 2 tunnels are made of two straight lines at right angles. When this is used with\n
 * short line segments it gives the "cavelike" tunnels seen deeper in the dungeon.\n
 *\n
 * Type 3 tunnels are made of two straight lines like type 2, but with extra rock removed.\n
 * This, when used with longer line segments gives the "catacomb-like" tunnels seen near\n
 * the surface.\n
 */
static void short_seg_hack(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int count, bool *fail)
{
    int i;
    POSITION x, y;
    int length;

    /* Check for early exit */
    if (!(*fail))
        return;

    length = distance(x1, y1, x2, y2);

    count++;

    if ((type == 1) && (length != 0)) {

        for (i = 0; i <= length; i++) {
            x = x1 + i * (x2 - x1) / length;
            y = y1 + i * (y2 - y1) / length;
            if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                if (count > 50) {
                    /* This isn't working - probably have an infinite loop */
                    *fail = FALSE;
                    return;
                }

                /* solid wall - so try to go around */
                short_seg_hack(player_ptr, x, y, x1 + (i - 1) * (x2 - x1) / length, y1 + (i - 1) * (y2 - y1) / length, 1, count, fail);
                short_seg_hack(player_ptr, x, y, x1 + (i + 1) * (x2 - x1) / length, y1 + (i + 1) * (y2 - y1) / length, 1, count, fail);
            }
        }
    } else if ((type == 2) || (type == 3)) {
        if (x1 < x2) {
            for (i = x1; i <= x2; i++) {
                x = i;
                y = y1;
                if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                    /* solid wall - so try to go around */
                    short_seg_hack(player_ptr, x, y, i - 1, y1, 1, count, fail);
                    short_seg_hack(player_ptr, x, y, i + 1, y1, 1, count, fail);
                }
                if ((type == 3) && ((x + y) % 2)) {
                    create_cata_tunnel(player_ptr, i, y1);
                }
            }
        } else {
            for (i = x2; i <= x1; i++) {
                x = i;
                y = y1;
                if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                    /* solid wall - so try to go around */
                    short_seg_hack(player_ptr, x, y, i - 1, y1, 1, count, fail);
                    short_seg_hack(player_ptr, x, y, i + 1, y1, 1, count, fail);
                }
                if ((type == 3) && ((x + y) % 2)) {
                    create_cata_tunnel(player_ptr, i, y1);
                }
            }
        }
        if (y1 < y2) {
            for (i = y1; i <= y2; i++) {
                x = x2;
                y = i;
                if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                    /* solid wall - so try to go around */
                    short_seg_hack(player_ptr, x, y, x2, i - 1, 1, count, fail);
                    short_seg_hack(player_ptr, x, y, x2, i + 1, 1, count, fail);
                }
                if ((type == 3) && ((x + y) % 2)) {
                    create_cata_tunnel(player_ptr, x2, i);
                }
            }
        } else {
            for (i = y2; i <= y1; i++) {
                x = x2;
                y = i;
                if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                    /* solid wall - so try to go around */
                    short_seg_hack(player_ptr, x, y, x2, i - 1, 1, count, fail);
                    short_seg_hack(player_ptr, x, y, x2, i + 1, 1, count, fail);
                }
                if ((type == 3) && ((x + y) % 2)) {
                    create_cata_tunnel(player_ptr, x2, i);
                }
            }
        }
    }
}

/*!
 * @brief 特定の壁(永久壁など)を避けながら部屋間の通路を作成する / This routine maps a path from (x1, y1) to (x2, y2) avoiding SOLID walls.
 * @return なし
 * @todo 詳細要調査
 * @details
 * Permanent rock is ignored in this path finding- sometimes there is no\n
 * path around anyway -so there will be a crash if we try to find one.\n
 * This routine is much like the river creation routine in Zangband.\n
 * It works by dividing a line segment into two.  The segments are divided\n
 * until they are less than "cutoff" - when the corresponding routine from\n
 * "short_seg_hack" is called.\n
 * Note it is VERY important that the "stop if hit another passage" logic\n
 * stays as is.  Without this the dungeon turns into Swiss Cheese...\n
 */
bool build_tunnel2(player_type *player_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, int type, int cutoff)
{
    POSITION x3, y3, dx, dy;
    POSITION changex, changey;
    int length;
    int i;
    bool retval, firstsuccede;
    grid_type *g_ptr;

    length = distance(x1, y1, x2, y2);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (length > cutoff) {
        /*
         * Divide path in half and call routine twice.
         */
        dx = (x2 - x1) / 2;
        dy = (y2 - y1) / 2;

        /* perturbation perpendicular to path */
        changex = (randint0(abs(dy) + 2) * 2 - abs(dy) - 1) / 2;
        changey = (randint0(abs(dx) + 2) * 2 - abs(dx) - 1) / 2;

        /* Work out "mid" ponit */
        x3 = x1 + dx + changex;
        y3 = y1 + dy + changey;

        /* See if in bounds - if not - do not perturb point */
        if (!in_bounds(floor_ptr, y3, x3)) {
            x3 = (x1 + x2) / 2;
            y3 = (y1 + y2) / 2;
        }
        /* cache g_ptr */
        g_ptr = &floor_ptr->grid_array[y3][x3];
        if (is_solid_grid(g_ptr)) {
            /* move midpoint a bit to avoid problem. */

            i = 50;

            dy = 0;
            dx = 0;
            while ((i > 0) && is_solid_bold(floor_ptr, y3 + dy, x3 + dx)) {
                dy = randint0(3) - 1;
                dx = randint0(3) - 1;
                if (!in_bounds(floor_ptr, y3 + dy, x3 + dx)) {
                    dx = 0;
                    dy = 0;
                }
                i--;
            }

            if (i == 0) {
                /* Failed for some reason: hack - ignore the solidness */
                place_bold(player_ptr, y3, x3, GB_OUTER);
                dx = 0;
                dy = 0;
            }
            y3 += dy;
            x3 += dx;
            g_ptr = &floor_ptr->grid_array[y3][x3];
        }

        if (is_floor_grid(g_ptr)) {
            if (build_tunnel2(player_ptr, x1, y1, x3, y3, type, cutoff)) {
                if ((floor_ptr->grid_array[y3][x3].info & CAVE_ROOM) || (randint1(100) > 95)) {
                    /* do second half only if works + if have hit a room */
                    retval = build_tunnel2(player_ptr, x3, y3, x2, y2, type, cutoff);
                } else {
                    /* have hit another tunnel - make a set of doors here */
                    retval = FALSE;

                    /* Save the door location */
                    if (dun_data->door_n < DOOR_MAX) {
                        dun_data->door[dun_data->door_n].y = y3;
                        dun_data->door[dun_data->door_n].x = x3;
                        dun_data->door_n++;
                    } else
                        return FALSE;
                }
                firstsuccede = TRUE;
            } else {
                /* false- didn't work all the way */
                retval = FALSE;
                firstsuccede = FALSE;
            }
        } else {
            /* tunnel through walls */
            if (build_tunnel2(player_ptr, x1, y1, x3, y3, type, cutoff)) {
                retval = build_tunnel2(player_ptr, x3, y3, x2, y2, type, cutoff);
                firstsuccede = TRUE;
            } else {
                /* false- didn't work all the way */
                retval = FALSE;
                firstsuccede = FALSE;
            }
        }
        if (firstsuccede) {
            /* only do this if the first half has worked */
            set_tunnel(player_ptr, &x3, &y3, TRUE);
        }
        /* return value calculated above */
        return retval;
    } else {
        /* Do a short segment */
        retval = TRUE;
        short_seg_hack(player_ptr, x1, y1, x2, y2, type, 0, &retval);

        /* Hack - ignore return value so avoid infinite loops */
        return TRUE;
    }
}
