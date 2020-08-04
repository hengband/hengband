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

#include "floor/floor-generator.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-dump.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/dungeon-tunnel-util.h"
#include "floor/cave-generator.h"
#include "floor/floor-allocation-types.h"
#include "floor/floor-events.h"
#include "floor/floor-generator-util.h"
#include "floor/floor-save.h"
#include "floor/floor-streams.h"
#include "floor/floor.h" // todo 相互依存している、後で消す.
#include "floor/object-allocator.h"
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
#include "room/room-generator.h"
#include "room/rooms-builder.h"
#include "room/rooms-maze-vault.h"
#include "system/dungeon-data-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

/*!
 * @brief クエストに関わるモンスターの配置を行う / Place quest monsters
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 成功したならばTRUEを返す
 */
bool place_quest_monsters(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (int i = 0; i < max_q_idx; i++) {
        monster_race *r_ptr;
        BIT_FLAGS mode;
        if (quest[i].status != QUEST_STATUS_TAKEN || (quest[i].type != QUEST_TYPE_KILL_LEVEL && quest[i].type != QUEST_TYPE_RANDOM)
            || quest[i].level != floor_ptr->dun_level || creature_ptr->dungeon_idx != quest[i].dungeon || (quest[i].flags & QUEST_FLAG_PRESET)) {
            continue;
        }

        r_ptr = &r_info[quest[i].r_idx];
        if ((r_ptr->flags1 & RF1_UNIQUE) && (r_ptr->cur_num >= r_ptr->max_num))
            continue;

        mode = (PM_NO_KAGE | PM_NO_PET);
        if (!(r_ptr->flags1 & RF1_FRIENDS))
            mode |= PM_ALLOW_GROUP;

        for (int j = 0; j < (quest[i].max_num - quest[i].cur_num); j++) {
            int k;
            for (k = 0; k < SAFE_MAX_ATTEMPTS; k++) {
                POSITION x = 0;
                POSITION y = 0;
                int l;
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

                if (l == 0)
                    return FALSE;

                if (place_monster_aux(creature_ptr, 0, y, x, quest[i].r_idx, mode))
                    break;
                else
                    continue;
            }

            if (k == SAFE_MAX_ATTEMPTS)
                return FALSE;
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
    POSITION yval = SCREEN_HGT / 2;
    POSITION xval = SCREEN_WID / 2;
    POSITION y_height = yval - 10;
    POSITION y_depth = yval + 10;
    POSITION x_left = xval - 32;
    POSITION x_right = xval + 32;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION i = y_height; i <= y_height + 5; i++)
        for (POSITION j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    for (POSITION i = y_depth; i >= y_depth - 5; i--)
        for (POSITION j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    for (POSITION j = x_left; j <= x_left + 17; j++)
        for (POSITION i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    for (POSITION j = x_right; j >= x_right - 17; j--)
        for (POSITION i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    place_bold(player_ptr, y_height + 6, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_left + 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_depth - 6, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 6][x_left + 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_height + 6, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_right - 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_depth - 6, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 6][x_right - 18].info |= CAVE_GLOW | CAVE_MARK;

    *start_y = y_height + 5;
    *start_x = xval;
    floor_ptr->grid_array[*start_y][*start_x].feat = f_tag_to_index("ARENA_GATE");
    floor_ptr->grid_array[*start_y][*start_x].info |= CAVE_GLOW | CAVE_MARK;
}

/*!
 * @brief 挑戦時闘技場への入場処理 / Town logic flow for generation of arena -KMW-
 * @return なし
 */
static void generate_challenge_arena(player_type *challanger_ptr)
{
    POSITION qy = 0;
    POSITION qx = 0;
    floor_type *floor_ptr = challanger_ptr->current_floor_ptr;
    floor_ptr->height = SCREEN_HGT;
    floor_ptr->width = SCREEN_WID;

    POSITION y, x;
    for (y = 0; y < MAX_HGT; y++)
        for (x = 0; x < MAX_WID; x++) {
            place_bold(challanger_ptr, y, x, GB_SOLID_PERM);
            floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
        }

    for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++)
        for (x = qx + 1; x < qx + SCREEN_WID - 1; x++)
            floor_ptr->grid_array[y][x].feat = feat_floor;

    build_arena(challanger_ptr, &y, &x);
    player_place(challanger_ptr, y, x);
    if (place_monster_aux(challanger_ptr, 0, challanger_ptr->y + 5, challanger_ptr->x, arena_info[challanger_ptr->arena_number].r_idx, PM_NO_KAGE | PM_NO_PET))
        return;

    challanger_ptr->exit_bldg = TRUE;
    challanger_ptr->arena_number++;
    msg_print(_("相手は欠場した。あなたの不戦勝だ。", "The enemy is unable appear. You won by default."));
}

/*!
 * @brief モンスター闘技場のフロア生成 / Builds the arena after it is entered -KMW-
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void build_battle(player_type *player_ptr, POSITION *y, POSITION *x)
{
    POSITION yval = SCREEN_HGT / 2;
    POSITION xval = SCREEN_WID / 2;
    POSITION y_height = yval - 10;
    POSITION y_depth = yval + 10;
    POSITION x_left = xval - 32;
    POSITION x_right = xval + 32;

    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = y_height; i <= y_height + 5; i++)
        for (int j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    for (int i = y_depth; i >= y_depth - 3; i--)
        for (int j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    for (int j = x_left; j <= x_left + 17; j++)
        for (int i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    for (int j = x_right; j >= x_right - 17; j--)
        for (int i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }

    place_bold(player_ptr, y_height + 6, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_left + 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_depth - 4, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 4][x_left + 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_height + 6, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_right - 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_depth - 4, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 4][x_right - 18].info |= CAVE_GLOW | CAVE_MARK;

    for (int i = y_height + 1; i <= y_height + 5; i++)
        for (int j = x_left + 20 + 2 * (y_height + 5 - i); j <= x_right - 20 - 2 * (y_height + 5 - i); j++) {
            floor_ptr->grid_array[i][j].feat = feat_permanent_glass_wall;
        }

    POSITION last_y = y_height + 1;
    POSITION last_x = xval;
    floor_ptr->grid_array[last_y][last_x].feat = f_tag_to_index("BUILDING_3");
    floor_ptr->grid_array[last_y][last_x].info |= CAVE_GLOW | CAVE_MARK;
    *y = last_y;
    *x = last_x;
}

/*!
 * @brief モンスター闘技場への導入処理 / Town logic flow for generation of arena -KMW-
 * @return なし
 */
static void generate_gambling_arena(player_type *creature_ptr)
{
    POSITION y, x;
    POSITION qy = 0;
    POSITION qx = 0;
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    for (y = 0; y < MAX_HGT; y++)
        for (x = 0; x < MAX_WID; x++) {
            place_bold(creature_ptr, y, x, GB_SOLID_PERM);
            floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
        }

    for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++)
        for (x = qx + 1; x < qx + SCREEN_WID - 1; x++)
            floor_ptr->grid_array[y][x].feat = feat_floor;

    build_battle(creature_ptr, &y, &x);
    player_place(creature_ptr, y, x);
    for (MONSTER_IDX i = 0; i < 4; i++) {
        place_monster_aux(creature_ptr, 0, creature_ptr->y + 8 + (i / 2) * 4, creature_ptr->x - 2 + (i % 2) * 4, battle_mon[i], (PM_NO_KAGE | PM_NO_PET));
        set_friendly(&floor_ptr->m_list[floor_ptr->grid_array[creature_ptr->y + 8 + (i / 2) * 4][creature_ptr->x - 2 + (i % 2) * 4].m_idx]);
    }

    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
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
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION y = 0; y < floor_ptr->height; y++)
        for (POSITION x = 0; x < floor_ptr->width; x++)
            place_bold(player_ptr, y, x, GB_SOLID_PERM);

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
        int level_height;
        int level_width;
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
        floor_ptr->height = MAX_HGT;
        floor_ptr->width = MAX_WID;
        panel_row_min = floor_ptr->height;
        panel_col_min = floor_ptr->width;
    }

    return cave_gen(player_ptr, why);
}

/*!
 * @brief フロアに存在する全マスの記憶状態を初期化する / Wipe all unnecessary flags after grid_array generation
 * @return なし
 */
void wipe_generate_random_floor_flags(floor_type *floor_ptr)
{
    for (POSITION y = 0; y < floor_ptr->height; y++)
        for (POSITION x = 0; x < floor_ptr->width; x++)
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);

    if (floor_ptr->dun_level > 0)
        for (POSITION y = 1; y < floor_ptr->height - 1; y++)
            for (POSITION x = 1; x < floor_ptr->width - 1; x++)
                floor_ptr->grid_array[y][x].info |= CAVE_UNSAFE;
}

/*!
 * @brief フロアの全情報を初期化する / Clear and empty floor.
 * @parama player_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void clear_cave(player_type *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    (void)C_WIPE(floor_ptr->o_list, floor_ptr->o_max, object_type);
    floor_ptr->o_max = 1;
    floor_ptr->o_cnt = 0;

    for (int i = 1; i < max_r_idx; i++)
        r_info[i].cur_num = 0;

    (void)C_WIPE(floor_ptr->m_list, floor_ptr->m_max, monster_type);
    floor_ptr->m_max = 1;
    floor_ptr->m_cnt = 0;
    for (int i = 0; i < MAX_MTIMED; i++)
        floor_ptr->mproc_max[i] = 0;

    precalc_cur_num_of_pet(player_ptr);
    for (POSITION y = 0; y < MAX_HGT; y++) {
        for (POSITION x = 0; x < MAX_WID; x++) {
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

    floor_ptr->base_level = floor_ptr->dun_level;
    floor_ptr->monster_level = floor_ptr->base_level;
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
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->dungeon_idx = player_ptr->dungeon_idx;
    set_floor_and_wall(floor_ptr->dungeon_idx);
    for (int num = 0; TRUE; num++) {
        bool okay = TRUE;
        concptr why = NULL;
        clear_cave(player_ptr);
        player_ptr->x = player_ptr->y = 0;
        if (floor_ptr->inside_arena)
            generate_challenge_arena(player_ptr);
        else if (player_ptr->phase_out)
            generate_gambling_arena(player_ptr);
        else if (floor_ptr->inside_quest)
            generate_fixed_floor(player_ptr);
        else if (!floor_ptr->dun_level)
            if (player_ptr->wild_mode)
                wilderness_gen_small(player_ptr);
            else
                wilderness_gen(player_ptr);
        else
            okay = level_gen(player_ptr, &why);

        if (floor_ptr->o_max >= current_world_ptr->max_o_idx) {
            why = _("アイテムが多すぎる", "too many objects");
            okay = FALSE;
        } else if (floor_ptr->m_max >= current_world_ptr->max_m_idx) {
            why = _("モンスターが多すぎる", "too many monsters");
            okay = FALSE;
        }

        if (okay)
            break;

        if (why)
            msg_format(_("生成やり直し(%s)", "Generation restarted (%s)"), why);

        wipe_o_list(floor_ptr);
        wipe_monsters_list(player_ptr);
    }

    glow_deep_lava_and_bldg(player_ptr);
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
    int i = randint0(4);
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
    *rdir = (y1 == y2) ? 0 : (y1 < y2) ? 1 : -1;
    *cdir = (x1 == x2) ? 0 : (x1 < x2) ? 1 : -1;
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
bool build_tunnel(player_type *player_ptr, dt_type *dt_ptr, POSITION row1, POSITION col1, POSITION row2, POSITION col2)
{
    POSITION tmp_row, tmp_col;
    POSITION row_dir, col_dir;
    POSITION start_row, start_col;
    int main_loop_count = 0;
    bool door_flag = FALSE;
    grid_type *g_ptr;
    start_row = row1;
    start_col = col1;
    correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    while ((row1 != row2) || (col1 != col2)) {
        if (main_loop_count++ > 2000)
            return FALSE;

        if (randint0(100) < dt_ptr->dun_tun_chg) {
            correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
            if (randint0(100) < dt_ptr->dun_tun_rnd)
                rand_dir(&row_dir, &col_dir);
        }

        tmp_row = row1 + row_dir;
        tmp_col = col1 + col_dir;
        while (!in_bounds(floor_ptr, tmp_row, tmp_col)) {
            correct_dir(&row_dir, &col_dir, row1, col1, row2, col2);
            if (randint0(100) < dt_ptr->dun_tun_rnd)
                rand_dir(&row_dir, &col_dir);

            tmp_row = row1 + row_dir;
            tmp_col = col1 + col_dir;
        }

        g_ptr = &floor_ptr->grid_array[tmp_row][tmp_col];
        if (is_solid_grid(g_ptr))
            continue;

        if (is_outer_grid(g_ptr)) {
            POSITION y = tmp_row + row_dir;
            POSITION x = tmp_col + col_dir;
            if (is_outer_bold(floor_ptr, y, x) || is_solid_bold(floor_ptr, y, x))
                continue;

            row1 = tmp_row;
            col1 = tmp_col;
            if (dun_data->wall_n >= WALL_MAX)
                return FALSE;

            dun_data->wall[dun_data->wall_n].y = row1;
            dun_data->wall[dun_data->wall_n].x = col1;
            dun_data->wall_n++;
            for (y = row1 - 1; y <= row1 + 1; y++)
                for (x = col1 - 1; x <= col1 + 1; x++)
                    if (is_outer_bold(floor_ptr, y, x))
                        place_bold(player_ptr, y, x, GB_SOLID_NOPERM);

        } else if (g_ptr->info & (CAVE_ROOM)) {
            row1 = tmp_row;
            col1 = tmp_col;
        } else if (is_extra_grid(g_ptr) || is_inner_grid(g_ptr) || is_solid_grid(g_ptr)) {
            row1 = tmp_row;
            col1 = tmp_col;
            if (dun_data->tunn_n >= TUNN_MAX)
                return FALSE;

            dun_data->tunn[dun_data->tunn_n].y = row1;
            dun_data->tunn[dun_data->tunn_n].x = col1;
            dun_data->tunn_n++;
            door_flag = FALSE;
        } else {
            row1 = tmp_row;
            col1 = tmp_col;
            if (!door_flag) {
                if (dun_data->door_n >= DOOR_MAX)
                    return FALSE;

                dun_data->door[dun_data->door_n].y = row1;
                dun_data->door[dun_data->door_n].x = col1;
                dun_data->door_n++;
                door_flag = TRUE;
            }

            if (randint0(100) >= dt_ptr->dun_tun_con) {
                tmp_row = row1 - start_row;
                if (tmp_row < 0)
                    tmp_row = (-tmp_row);

                tmp_col = col1 - start_col;
                if (tmp_col < 0)
                    tmp_col = (-tmp_col);

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
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    grid_type *g_ptr = &floor_ptr->grid_array[*y][*x];
    if (!in_bounds(floor_ptr, *y, *x) || is_inner_grid(g_ptr))
        return TRUE;

    if (is_extra_bold(floor_ptr, *y, *x)) {
        if (dun_data->tunn_n >= TUNN_MAX)
            return FALSE;

        dun_data->tunn[dun_data->tunn_n].y = *y;
        dun_data->tunn[dun_data->tunn_n].x = *x;
        dun_data->tunn_n++;
        return TRUE;
    }

    if (is_floor_bold(floor_ptr, *y, *x))
        return TRUE;

    if (is_outer_grid(g_ptr) && affectwall) {
        if (dun_data->wall_n >= WALL_MAX)
            return FALSE;

        dun_data->wall[dun_data->wall_n].y = *y;
        dun_data->wall[dun_data->wall_n].x = *x;
        dun_data->wall_n++;
        for (int j = *y - 1; j <= *y + 1; j++)
            for (int i = *x - 1; i <= *x + 1; i++)
                if (is_outer_bold(floor_ptr, j, i))
                    place_bold(player_ptr, j, i, GB_SOLID_NOPERM);

        floor_ptr->grid_array[*y][*x].mimic = 0;
        place_bold(player_ptr, *y, *x, GB_FLOOR);
        return TRUE;
    }

    if (is_solid_grid(g_ptr) && affectwall) {
        int i = 50;
        int dy = 0;
        int dx = 0;
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
            place_grid(player_ptr, g_ptr, GB_OUTER);
            dx = 0;
            dy = 0;
        }

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
    POSITION x1 = x - 1;
    POSITION y1 = y;
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
static void short_seg_hack(player_type *player_ptr, const POSITION x1, const POSITION y1, const POSITION x2, const POSITION y2, int type, int count, bool *fail)
{
    if (!(*fail))
        return;

    int length = distance(x1, y1, x2, y2);
    count++;
    POSITION x, y;
    if ((type == 1) && (length != 0)) {
        for (int i = 0; i <= length; i++) {
            x = x1 + i * (x2 - x1) / length;
            y = y1 + i * (y2 - y1) / length;
            if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                if (count > 50) {
                    *fail = FALSE;
                    return;
                }

                short_seg_hack(player_ptr, x, y, x1 + (i - 1) * (x2 - x1) / length, y1 + (i - 1) * (y2 - y1) / length, 1, count, fail);
                short_seg_hack(player_ptr, x, y, x1 + (i + 1) * (x2 - x1) / length, y1 + (i + 1) * (y2 - y1) / length, 1, count, fail);
            }
        }

        return;
    }

    if ((type != 2) && (type != 3))
        return;

    if (x1 < x2) {
        for (int i = x1; i <= x2; i++) {
            x = i;
            y = y1;
            if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                short_seg_hack(player_ptr, x, y, i - 1, y1, 1, count, fail);
                short_seg_hack(player_ptr, x, y, i + 1, y1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2))
                create_cata_tunnel(player_ptr, i, y1);
        }
    } else {
        for (int i = x2; i <= x1; i++) {
            x = i;
            y = y1;
            if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                short_seg_hack(player_ptr, x, y, i - 1, y1, 1, count, fail);
                short_seg_hack(player_ptr, x, y, i + 1, y1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2))
                create_cata_tunnel(player_ptr, i, y1);
        }
    }

    if (y1 < y2) {
        for (int i = y1; i <= y2; i++) {
            x = x2;
            y = i;
            if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                short_seg_hack(player_ptr, x, y, x2, i - 1, 1, count, fail);
                short_seg_hack(player_ptr, x, y, x2, i + 1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2))
                create_cata_tunnel(player_ptr, x2, i);
        }
    } else {
        for (int i = y2; i <= y1; i++) {
            x = x2;
            y = i;
            if (!set_tunnel(player_ptr, &x, &y, TRUE)) {
                short_seg_hack(player_ptr, x, y, x2, i - 1, 1, count, fail);
                short_seg_hack(player_ptr, x, y, x2, i + 1, 1, count, fail);
            }

            if ((type == 3) && ((x + y) % 2))
                create_cata_tunnel(player_ptr, x2, i);
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
    bool retval, firstsuccede;
    grid_type *g_ptr;

    int length = distance(x1, y1, x2, y2);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (length <= cutoff) {
        retval = TRUE;
        short_seg_hack(player_ptr, x1, y1, x2, y2, type, 0, &retval);
        return TRUE;
    }

    dx = (x2 - x1) / 2;
    dy = (y2 - y1) / 2;
    changex = (randint0(abs(dy) + 2) * 2 - abs(dy) - 1) / 2;
    changey = (randint0(abs(dx) + 2) * 2 - abs(dx) - 1) / 2;
    x3 = x1 + dx + changex;
    y3 = y1 + dy + changey;
    if (!in_bounds(floor_ptr, y3, x3)) {
        x3 = (x1 + x2) / 2;
        y3 = (y1 + y2) / 2;
    }

    g_ptr = &floor_ptr->grid_array[y3][x3];
    if (is_solid_grid(g_ptr)) {
        int i = 50;
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
                retval = build_tunnel2(player_ptr, x3, y3, x2, y2, type, cutoff);
            } else {
                retval = FALSE;
                if (dun_data->door_n >= DOOR_MAX)
                    return FALSE;

                dun_data->door[dun_data->door_n].y = y3;
                dun_data->door[dun_data->door_n].x = x3;
                dun_data->door_n++;
            }

            firstsuccede = TRUE;
        } else {
            retval = FALSE;
            firstsuccede = FALSE;
        }
    } else {
        if (build_tunnel2(player_ptr, x1, y1, x3, y3, type, cutoff)) {
            retval = build_tunnel2(player_ptr, x3, y3, x2, y2, type, cutoff);
            firstsuccede = TRUE;
        } else {
            retval = FALSE;
            firstsuccede = FALSE;
        }
    }

    if (firstsuccede)
        set_tunnel(player_ptr, &x3, &y3, TRUE);

    return retval;
}
