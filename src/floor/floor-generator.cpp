﻿/*!
 * @brief ダンジョンの生成 / Dungeon generation
 * @date 2014/01/04
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * 2014 Deskull rearranged comment for Doxygen. \n
 */

#include <assert.h>

#include "dungeon/dungeon-flag-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave-generator.h"
#include "floor/floor-events.h"
#include "floor/floor-generator.h"
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
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"

//--------------------------------------------------------------------
// 動的可変長配列
//--------------------------------------------------------------------

typedef struct Vec {
    size_t len;
    size_t cap;
    int *data;
} Vec;

// 容量 cap の空 Vec を返す。
static Vec vec_with_capacity(const size_t cap)
{
    assert(cap > 0);

    Vec vec = { .len = 0, .cap = cap, .data = NULL };
    vec.data = static_cast<int*>(malloc(sizeof(int) * cap));
    if (!vec.data)
        rpanic(cap);

    return vec;
}

// サイズ len で、全要素が init で初期化された Vec を返す。
static Vec vec_new(const size_t len, const int init)
{
    assert(len > 0);

    const size_t cap = 2 * len;
    Vec vec = vec_with_capacity(cap);

    vec.len = len;
    for (size_t i = 0; i < len; ++i)
        vec.data[i] = init;

    return vec;
}

static void vec_delete(Vec *const vec)
{
    free(vec->data);

    vec->len = 0;
    vec->cap = 0;
    vec->data = NULL;
}

static size_t vec_size(const Vec *const vec) { return vec->len; }

static bool vec_is_empty(const Vec *const vec) { return vec_size(vec) == 0; }

static void vec_push_back(Vec *const vec, const int e)
{
    // 容量不足になったら容量を拡張する。
    if (vec->len == vec->cap) {
        vec->cap = vec->cap > 0 ? 2 * vec->cap : 1;
        vec->data = static_cast<int*>(realloc(vec->data, sizeof(int) * vec->cap));
        if (!vec->data)
            rpanic(vec->cap);
    }

    vec->data[vec->len++] = e;
}

static int vec_pop_back(Vec *const vec)
{
    assert(!vec_is_empty(vec));

    return vec->data[vec->len-- - 1];
}

//--------------------------------------------------------------------

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
    msg_print(_("相手は欠場した。あなたの不戦勝だ。", "The enemy is unable to appear. You won by default."));
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

typedef bool (*IsWallFunc)(const floor_type *, int, int);

// (y,x) がプレイヤーが通れない永久地形かどうかを返す。
static bool is_permanent_blocker(const floor_type *const floor_ptr, const int y, const int x)
{
    const FEAT_IDX feat = floor_ptr->grid_array[y][x].feat;
    const BIT_FLAGS *const flags = f_info[feat].flags;
    return has_flag(flags, FF_PERMANENT) && !has_flag(flags, FF_MOVE);
}

static void floor_is_connected_dfs(
    const floor_type *const floor_ptr, const IsWallFunc is_wall, const int y_start, const int x_start, Vec *const stk, const Vec *const visited)
{
    // clang-format off
    static const int DY[8] = { -1, -1, -1,  0, 0,  1, 1, 1 };
    static const int DX[8] = { -1,  0,  1, -1, 1, -1, 0, 1 };
    // clang-format on

    const int h = floor_ptr->height;
    const int w = floor_ptr->width;
    const int start = w * y_start + x_start;

    vec_push_back(stk, start);
    visited->data[start] = 1;

    while (!vec_is_empty(stk)) {
        const int cur = vec_pop_back(stk);
        const int y = cur / w;
        const int x = cur % w;

        for (int i = 0; i < 8; ++i) {
            const int y_nxt = y + DY[i];
            const int x_nxt = x + DX[i];
            if (y_nxt < 0 || h <= y_nxt || x_nxt < 0 || w <= x_nxt)
                continue;
            const int nxt = w * y_nxt + x_nxt;
            if (visited->data[nxt])
                continue;
            if (is_wall(floor_ptr, y_nxt, x_nxt))
                continue;

            vec_push_back(stk, nxt);
            visited->data[nxt] = 1;
        }
    }
}

// 現在のフロアが連結かどうかを返す。
// 各セルの8近傍は互いに移動可能とし、is_wall が真を返すセルのみを壁とみなす。
//
// 連結成分数が 0 の場合、偽を返す。
static bool floor_is_connected(const floor_type *const floor_ptr, const IsWallFunc is_wall)
{
    const int h = floor_ptr->height;
    const int w = floor_ptr->width;

    // ヒープ上に確保したスタックを用いてDFSする。
    // 最大フロアサイズが h=66, w=198 なので、単純に再帰DFSするとスタックオーバーフローが不安。
    Vec stk = vec_with_capacity(1024);
    Vec visited = vec_new((size_t)h * (size_t)w, 0);
    int n_component = 0; // 連結成分数

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int idx = w * y + x;
            if (visited.data[idx])
                continue;
            if (is_wall(floor_ptr, y, x))
                continue;

            if (++n_component >= 2)
                goto finish;
            floor_is_connected_dfs(floor_ptr, is_wall, y, x, &stk, &visited);
        }
    }

finish:
    vec_delete(&visited);
    vec_delete(&stk);

    return n_component == 1;
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

        // ダンジョン内フロアが連結でない(永久壁で区切られた孤立部屋がある)場合、
        // 狂戦士でのプレイに支障をきたしうるので再生成する。
        // 地上、荒野マップ、クエストでは連結性判定は行わない。
        // TODO: 本来はダンジョン生成アルゴリズム自身で連結性を保証するのが理想ではある。
        const bool check_conn = okay && floor_ptr->dun_level > 0 && floor_ptr->inside_quest == 0;
        if (check_conn && !floor_is_connected(floor_ptr, is_permanent_blocker)) {
            // 一定回数試しても連結にならないなら諦める。
            if (num >= 1000) {
                plog("cannot generate connected floor. giving up...");
            } else {
                why = _("フロアが連結でない", "floor is not connected");
                okay = FALSE;
            }
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
