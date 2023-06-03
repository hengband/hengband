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
#include "dungeon/quest.h"
#include "floor/cave-generator.h"
#include "floor/floor-events.h"
#include "floor/floor-generator.h"
#include "floor/floor-save.h" //!< @todo precalc_cur_num_of_pet() が依存している、違和感.
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
#include "player/player-status.h"
#include "system/building-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "wizard/wizard-messages.h"
#include "world/world.h"
#include <algorithm>
#include <array>
#include <stack>

/*!
 * @brief 闘技場用のアリーナ地形を作成する / Builds the on_defeat_arena_monster after it is entered -KMW-
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void build_arena(PlayerType *player_ptr, POSITION *start_y, POSITION *start_x)
{
    POSITION yval = SCREEN_HGT / 2;
    POSITION xval = SCREEN_WID / 2;
    POSITION y_height = yval - 10;
    POSITION y_depth = yval + 10;
    POSITION x_left = xval - 32;
    POSITION x_right = xval + 32;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION i = y_height; i <= y_height + 5; i++) {
        for (POSITION j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    for (POSITION i = y_depth; i >= y_depth - 5; i--) {
        for (POSITION j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    for (POSITION j = x_left; j <= x_left + 17; j++) {
        for (POSITION i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    for (POSITION j = x_right; j >= x_right - 17; j--) {
        for (POSITION i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
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
 */
static void generate_challenge_arena(PlayerType *player_ptr)
{
    POSITION qy = 0;
    POSITION qx = 0;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->height = SCREEN_HGT;
    floor_ptr->width = SCREEN_WID;

    POSITION y, x;
    for (y = 0; y < MAX_HGT; y++) {
        for (x = 0; x < MAX_WID; x++) {
            place_bold(player_ptr, y, x, GB_SOLID_PERM);
            floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++) {
        for (x = qx + 1; x < qx + SCREEN_WID - 1; x++) {
            floor_ptr->grid_array[y][x].feat = feat_floor;
        }
    }

    build_arena(player_ptr, &y, &x);
    player_place(player_ptr, y, x);
    if (place_monster_aux(player_ptr, 0, player_ptr->y + 5, player_ptr->x, arena_info[player_ptr->arena_number].r_idx, PM_NO_KAGE | PM_NO_PET)) {
        return;
    }

    player_ptr->exit_bldg = true;
    player_ptr->arena_number++;
    msg_print(_("相手は欠場した。あなたの不戦勝だ。", "The enemy is unable to appear. You won by default."));
}

/*!
 * @brief モンスター闘技場のフロア生成 / Builds the on_defeat_arena_monster after it is entered -KMW-
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void build_battle(PlayerType *player_ptr, POSITION *y, POSITION *x)
{
    POSITION yval = SCREEN_HGT / 2;
    POSITION xval = SCREEN_WID / 2;
    POSITION y_height = yval - 10;
    POSITION y_depth = yval + 10;
    POSITION x_left = xval - 32;
    POSITION x_right = xval + 32;

    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = y_height; i <= y_height + 5; i++) {
        for (int j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    for (int i = y_depth; i >= y_depth - 3; i--) {
        for (int j = x_left; j <= x_right; j++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    for (int j = x_left; j <= x_left + 17; j++) {
        for (int i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    for (int j = x_right; j >= x_right - 17; j--) {
        for (int i = y_height; i <= y_depth; i++) {
            place_bold(player_ptr, i, j, GB_EXTRA_PERM);
            floor_ptr->grid_array[i][j].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    place_bold(player_ptr, y_height + 6, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_left + 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_depth - 4, x_left + 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 4][x_left + 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_height + 6, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_height + 6][x_right - 18].info |= CAVE_GLOW | CAVE_MARK;
    place_bold(player_ptr, y_depth - 4, x_right - 18, GB_EXTRA_PERM);
    floor_ptr->grid_array[y_depth - 4][x_right - 18].info |= CAVE_GLOW | CAVE_MARK;

    for (int i = y_height + 1; i <= y_height + 5; i++) {
        for (int j = x_left + 20 + 2 * (y_height + 5 - i); j <= x_right - 20 - 2 * (y_height + 5 - i); j++) {
            floor_ptr->grid_array[i][j].feat = feat_permanent_glass_wall;
        }
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
 */
static void generate_gambling_arena(PlayerType *player_ptr)
{
    POSITION y, x;
    POSITION qy = 0;
    POSITION qx = 0;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (y = 0; y < MAX_HGT; y++) {
        for (x = 0; x < MAX_WID; x++) {
            place_bold(player_ptr, y, x, GB_SOLID_PERM);
            floor_ptr->grid_array[y][x].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    for (y = qy + 1; y < qy + SCREEN_HGT - 1; y++) {
        for (x = qx + 1; x < qx + SCREEN_WID - 1; x++) {
            floor_ptr->grid_array[y][x].feat = feat_floor;
        }
    }

    build_battle(player_ptr, &y, &x);
    player_place(player_ptr, y, x);
    for (MONSTER_IDX i = 0; i < 4; i++) {
        place_monster_aux(player_ptr, 0, player_ptr->y + 8 + (i / 2) * 4, player_ptr->x - 2 + (i % 2) * 4, battle_mon_list[i], (PM_NO_KAGE | PM_NO_PET));
        set_friendly(&floor_ptr->m_list[floor_ptr->grid_array[player_ptr->y + 8 + (i / 2) * 4][player_ptr->x - 2 + (i % 2) * 4].m_idx]);
    }

    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        auto *m_ptr = &floor_ptr->m_list[i];
        if (!m_ptr->is_valid()) {
            continue;
        }

        m_ptr->mflag2.set({ MonsterConstantFlagType::MARK, MonsterConstantFlagType::SHOW });
        update_monster(player_ptr, i, false);
    }
}

/*!
 * @brief 固定マップクエストのフロア生成 / Generate a quest level
 * @param player_ptr プレイヤーへの参照ポインタ
 */
static void generate_fixed_floor(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            place_bold(player_ptr, y, x, GB_SOLID_PERM);
        }
    }

    const auto &quest_list = QuestList::get_instance();
    floor_ptr->base_level = quest_list[floor_ptr->quest_number].level;
    floor_ptr->dun_level = floor_ptr->base_level;
    floor_ptr->object_level = floor_ptr->base_level;
    floor_ptr->monster_level = floor_ptr->base_level;
    if (record_stair) {
        exe_write_diary_quest(player_ptr, DiaryKind::TO_QUEST, floor_ptr->quest_number);
    }
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), nullptr);
    init_flags = INIT_CREATE_DUNGEON;
    parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, MAX_HGT, MAX_WID);
}

/*!
 * @brief ダンジョン時のランダムフロア生成 / Make a real level
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param concptr
 * @return フロアの生成に成功したらTRUE
 */
static bool level_gen(PlayerType *player_ptr, concptr *why)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    DUNGEON_IDX d_idx = floor_ptr->dungeon_idx;
    const auto &dungeon = dungeons_info[d_idx];
    constexpr auto chance_small_floor = 3;
    auto is_small_level = always_small_levels || ironman_small_levels;
    is_small_level |= one_in_(chance_small_floor) && small_levels;
    is_small_level |= dungeon.flags.has(DungeonFeatureType::BEGINNER);
    is_small_level |= dungeon.flags.has(DungeonFeatureType::SMALLEST);
    if (is_small_level && dungeon.flags.has_not(DungeonFeatureType::BIG)) {
        int level_height;
        int level_width;
        if (dungeon.flags.has(DungeonFeatureType::SMALLEST)) {
            level_height = 1;
            level_width = 1;
        } else if (dungeon.flags.has(DungeonFeatureType::BEGINNER)) {
            level_height = 2;
            level_width = 2;
        } else {
            level_height = randint1(MAX_HGT / SCREEN_HGT);
            level_width = randint1(MAX_WID / SCREEN_WID);
            bool is_first_level_area = true;
            bool is_max_area = (level_height == MAX_HGT / SCREEN_HGT) && (level_width == MAX_WID / SCREEN_WID);
            while (is_first_level_area || is_max_area) {
                level_height = randint1(MAX_HGT / SCREEN_HGT);
                level_width = randint1(MAX_WID / SCREEN_WID);
                is_first_level_area = false;
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
 */
void wipe_generate_random_floor_flags(FloorType *floor_ptr)
{
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            floor_ptr->grid_array[y][x].info &= ~(CAVE_MASK);
        }
    }

    if (floor_ptr->dun_level > 0) {
        for (POSITION y = 1; y < floor_ptr->height - 1; y++) {
            for (POSITION x = 1; x < floor_ptr->width - 1; x++) {
                floor_ptr->grid_array[y][x].info |= CAVE_UNSAFE;
            }
        }
    }
}

/*!
 * @brief フロアの全情報を初期化する / Clear and empty floor.
 * @parama player_ptr プレイヤーへの参照ポインタ
 */
void clear_cave(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    std::fill_n(floor_ptr->o_list.begin(), floor_ptr->o_max, ItemEntity{});
    floor_ptr->o_max = 1;
    floor_ptr->o_cnt = 0;

    for (auto &[r_idx, r_ref] : monraces_info) {
        r_ref.cur_num = 0;
    }

    std::fill_n(floor_ptr->m_list.begin(), floor_ptr->m_max, MonsterEntity{});
    floor_ptr->m_max = 1;
    floor_ptr->m_cnt = 0;
    for (int i = 0; i < MAX_MTIMED; i++) {
        floor_ptr->mproc_max[i] = 0;
    }

    precalc_cur_num_of_pet(player_ptr);
    for (POSITION y = 0; y < MAX_HGT; y++) {
        for (POSITION x = 0; x < MAX_WID; x++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info = 0;
            g_ptr->feat = 0;
            g_ptr->o_idx_list.clear();
            g_ptr->m_idx = 0;
            g_ptr->special = 0;
            g_ptr->mimic = 0;
            g_ptr->reset_costs();
            g_ptr->reset_dists();
            g_ptr->when = 0;
        }
    }

    floor_ptr->base_level = floor_ptr->dun_level;
    floor_ptr->monster_level = floor_ptr->base_level;
    floor_ptr->object_level = floor_ptr->base_level;
}

typedef bool (*IsWallFunc)(const FloorType *, int, int);

// (y,x) がプレイヤーが通れない永久地形かどうかを返す。
static bool is_permanent_blocker(const FloorType *const floor_ptr, const int y, const int x)
{
    const FEAT_IDX feat = floor_ptr->grid_array[y][x].feat;
    const auto &flags = terrains_info[feat].flags;
    return flags.has(TerrainCharacteristics::PERMANENT) && flags.has_not(TerrainCharacteristics::MOVE);
}

static void floor_is_connected_dfs(const FloorType *const floor_ptr, const IsWallFunc is_wall, const int y_start, const int x_start, bool *const visited)
{
    // clang-format off
    static const int DY[8] = { -1, -1, -1,  0, 0,  1, 1, 1 };
    static const int DX[8] = { -1,  0,  1, -1, 1, -1, 0, 1 };
    // clang-format on

    const int h = floor_ptr->height;
    const int w = floor_ptr->width;
    const int start = w * y_start + x_start;

    // 深さ優先探索用のスタック。
    // 最大フロアサイズが h=66, w=198 なので、スタックオーバーフロー防止のため再帰は使わない。
    std::stack<int> stk;

    stk.emplace(start);
    visited[start] = true;

    while (!stk.empty()) {
        const int cur = stk.top();
        stk.pop();
        const int y = cur / w;
        const int x = cur % w;

        for (int i = 0; i < 8; ++i) {
            const int y_nxt = y + DY[i];
            const int x_nxt = x + DX[i];
            if (y_nxt < 0 || h <= y_nxt || x_nxt < 0 || w <= x_nxt) {
                continue;
            }
            const int nxt = w * y_nxt + x_nxt;
            if (visited[nxt]) {
                continue;
            }
            if (is_wall(floor_ptr, y_nxt, x_nxt)) {
                continue;
            }

            stk.emplace(nxt);
            visited[nxt] = true;
        }
    }
}

// 現在のフロアが連結かどうかを返す。
// 各セルの8近傍は互いに移動可能とし、is_wall が真を返すセルのみを壁とみなす。
//
// 連結成分数が 0 の場合、偽を返す。
static bool floor_is_connected(const FloorType *const floor_ptr, const IsWallFunc is_wall)
{
    static std::array<bool, MAX_HGT * MAX_WID> visited;

    const int h = floor_ptr->height;
    const int w = floor_ptr->width;

    std::fill(begin(visited), end(visited), false);

    int n_component = 0; // 連結成分数

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const int idx = w * y + x;
            if (visited[idx]) {
                continue;
            }
            if (is_wall(floor_ptr, y, x)) {
                continue;
            }

            if (++n_component >= 2) {
                break;
            }
            floor_is_connected_dfs(floor_ptr, is_wall, y, x, visited.data());
        }
    }

    return n_component == 1;
}

/*!
 * ダンジョンのランダムフロアを生成する / Generates a random dungeon level -RAK-
 * @parama player_ptr プレイヤーへの参照ポインタ
 * @note Hack -- regenerate any "overflow" levels
 */
void generate_floor(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    set_floor_and_wall(floor_ptr->dungeon_idx);
    for (int num = 0; true; num++) {
        bool okay = true;
        concptr why = nullptr;
        clear_cave(player_ptr);
        player_ptr->x = player_ptr->y = 0;
        if (floor_ptr->inside_arena) {
            generate_challenge_arena(player_ptr);
        } else if (player_ptr->phase_out) {
            generate_gambling_arena(player_ptr);
        } else if (inside_quest(floor_ptr->quest_number)) {
            generate_fixed_floor(player_ptr);
        } else if (!floor_ptr->dun_level) {
            if (player_ptr->wild_mode) {
                wilderness_gen_small(player_ptr);
            } else {
                wilderness_gen(player_ptr);
            }
        } else {
            okay = level_gen(player_ptr, &why);
        }

        if (floor_ptr->o_max >= w_ptr->max_o_idx) {
            why = _("アイテムが多すぎる", "too many objects");
            okay = false;
        } else if (floor_ptr->m_max >= w_ptr->max_m_idx) {
            why = _("モンスターが多すぎる", "too many monsters");
            okay = false;
        }

        // ダンジョン内フロアが連結でない(永久壁で区切られた孤立部屋がある)場合、
        // 狂戦士でのプレイに支障をきたしうるので再生成する。
        // 地上、荒野マップ、クエストでは連結性判定は行わない。
        // TODO: 本来はダンジョン生成アルゴリズム自身で連結性を保証するのが理想ではある。
        const bool check_conn = okay && floor_ptr->dun_level > 0 && !inside_quest(floor_ptr->quest_number);
        if (check_conn && !floor_is_connected(floor_ptr, is_permanent_blocker)) {
            // 一定回数試しても連結にならないなら諦める。
            if (num >= 1000) {
                plog("cannot generate connected floor. giving up...");
            } else {
                why = _("フロアが連結でない", "floor is not connected");
                okay = false;
            }
        }

        if (okay) {
            break;
        }

        if (why) {
            msg_format(_("生成やり直し(%s)", "Generation restarted (%s)"), why);
        }

        wipe_o_list(floor_ptr);
        wipe_monsters_list(player_ptr);
    }

    glow_deep_lava_and_bldg(player_ptr);
    player_ptr->enter_dungeon = false;
    wipe_generate_random_floor_flags(floor_ptr);
}
