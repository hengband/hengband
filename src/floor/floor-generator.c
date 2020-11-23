/*!
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
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave-generator.h"
#include "floor/floor-events.h"
#include "floor/floor-save.h" // todo precalc_cur_num_of_pet() が依存している、違和感.
#include "floor/floor-util.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "game-option/game-play-options.h"
#include "game-option/play-record-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/feature-reader.h"
#include "info-reader/fixed-map-parser.h"
#include "io/write-diary.h"
#include "market/arena-info-table.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/monster-util.h"
#include "system/building-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

/*!
 * @brief 闘技場用のアリーナ地形を作成する / Builds the on_defeat_arena_monster after it is entered -KMW-
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
 * @brief 挑戦時闘技場への入場処理 / Town logic flow for generation of on_defeat_arena_monster -KMW-
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
 * @brief モンスター闘技場のフロア生成 / Builds the on_defeat_arena_monster after it is entered -KMW-
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
 * @brief モンスター闘技場への導入処理 / Town logic flow for generation of on_defeat_arena_monster -KMW-
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

        m_ptr->mflag2 |= MFLAG2_MARK | MFLAG2_SHOW;
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
