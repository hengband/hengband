/*!
 * @brief 荒野マップの生成とルール管理 / Wilderness generation
 * @date 2014/02/13
 * @author
 * Copyright (c) 1989 James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 * 2013 Deskull rearranged comment for Doxygen.
 */

#include "floor/wild.h"
#include "core/asking-player.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-town.h"
#include "game-option/birth-options.h"
#include "game-option/map-screen-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "info-reader/fixed-map-parser.h"
#include "info-reader/parse-error-types.h"
#include "io/files-util.h"
#include "io/tokenizer.h"
#include "market/building-initializer.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-floor/place-monster-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/monster-util.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-status.h"
#include "realm/realm-names-table.h"
#include "spell-realm/spells-hex.h"
#include "status/action-setter.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "world/world.h"

#define MAX_FEAT_IN_TERRAIN 18

std::vector<std::vector<wilderness_type>> wilderness;
bool generate_encounter;

struct border_type {
    int16_t north[MAX_WID];
    int16_t south[MAX_WID];
    int16_t east[MAX_HGT];
    int16_t west[MAX_HGT];
    int16_t north_west;
    int16_t north_east;
    int16_t south_west;
    int16_t south_east;
};

static border_type border;

/*!
 * @brief 地形生成確率を決める要素100の配列を確率テーブルから作成する
 * @param feat_type 非一様確率を再現するための要素数100の配列
 * @param prob 元の確率テーブル
 */
static void set_floor_and_wall_aux(int16_t feat_type[100], const std::array<feat_prob, DUNGEON_FEAT_PROB_NUM> &prob)
{
    int lim[DUNGEON_FEAT_PROB_NUM];
    lim[0] = prob[0].percent;
    for (int i = 1; i < DUNGEON_FEAT_PROB_NUM; i++) {
        lim[i] = lim[i - 1] + prob[i].percent;
    }

    if (lim[DUNGEON_FEAT_PROB_NUM - 1] < 100) {
        lim[DUNGEON_FEAT_PROB_NUM - 1] = 100;
    }

    int cur = 0;
    for (int i = 0; i < 100; i++) {
        while (i == lim[cur]) {
            cur++;
        }

        feat_type[i] = prob[cur].feat;
    }
}

/*!
 * @brief ダンジョンの地形を指定確率に応じて各マスへランダムに敷き詰める
 * / Fill the arrays of floors and walls in the good proportions
 * @param type ダンジョンID
 */
void set_floor_and_wall(DUNGEON_IDX type)
{
    DUNGEON_IDX cur_type = 255;
    if (cur_type == type) {
        return;
    }

    cur_type = type;
    dungeon_type *d_ptr = &dungeons_info[type];

    set_floor_and_wall_aux(feat_ground_type, d_ptr->floor);
    set_floor_and_wall_aux(feat_wall_type, d_ptr->fill);

    feat_wall_outer = d_ptr->outer_wall;
    feat_wall_inner = d_ptr->inner_wall;
    feat_wall_solid = d_ptr->outer_wall;
}

/*!
 * @brief プラズマフラクタル的地形生成の再帰中間処理
 * / Helper for plasma generation.
 * @param x1 左上端の深み
 * @param x2 右上端の深み
 * @param x3 左下端の深み
 * @param x4 右下端の深み
 * @param xmid 中央座標X
 * @param ymid 中央座標Y
 * @param rough ランダム幅
 * @param depth_max 深みの最大値
 */
static void perturb_point_mid(
    FloorType *floor_ptr, FEAT_IDX x1, FEAT_IDX x2, FEAT_IDX x3, FEAT_IDX x4, POSITION xmid, POSITION ymid, FEAT_IDX rough, FEAT_IDX depth_max)
{
    FEAT_IDX tmp2 = rough * 2 + 1;
    FEAT_IDX tmp = randint1(tmp2) - (rough + 1);
    FEAT_IDX avg = ((x1 + x2 + x3 + x4) / 4) + tmp;
    if (((x1 + x2 + x3 + x4) % 4) > 1) {
        avg++;
    }

    if (avg < 0) {
        avg = 0;
    }

    if (avg > depth_max) {
        avg = depth_max;
    }

    floor_ptr->grid_array[ymid][xmid].feat = (FEAT_IDX)avg;
}

/*!
 * @brief プラズマフラクタル的地形生成の再帰末端処理
 * / Helper for plasma generation.
 * @param x1 中間末端部1の重み
 * @param x2 中間末端部2の重み
 * @param x3 中間末端部3の重み
 * @param xmid 最終末端部座標X
 * @param ymid 最終末端部座標Y
 * @param rough ランダム幅
 * @param depth_max 深みの最大値
 */
static void perturb_point_end(FloorType *floor_ptr, FEAT_IDX x1, FEAT_IDX x2, FEAT_IDX x3, POSITION xmid, POSITION ymid, FEAT_IDX rough, FEAT_IDX depth_max)
{
    FEAT_IDX tmp2 = rough * 2 + 1;
    FEAT_IDX tmp = randint0(tmp2) - rough;
    FEAT_IDX avg = ((x1 + x2 + x3) / 3) + tmp;
    if ((x1 + x2 + x3) % 3) {
        avg++;
    }

    if (avg < 0) {
        avg = 0;
    }

    if (avg > depth_max) {
        avg = depth_max;
    }

    floor_ptr->grid_array[ymid][xmid].feat = (FEAT_IDX)avg;
}

/*!
 * @brief プラズマフラクタル的地形生成の開始処理
 * / Helper for plasma generation.
 * @param x1 処理範囲の左上X座標
 * @param y1 処理範囲の左上Y座標
 * @param x2 処理範囲の右下X座標
 * @param y2 処理範囲の右下Y座標
 * @param depth_max 深みの最大値
 * @param rough ランダム幅
 * @details
 * <pre>
 * A generic function to generate the plasma fractal.
 * Note that it uses ``cave_feat'' as temporary storage.
 * The values in ``cave_feat'' after this function
 * are NOT actual features; They are raw heights which
 * need to be converted to features.
 * </pre>
 */
static void plasma_recursive(FloorType *floor_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, FEAT_IDX depth_max, FEAT_IDX rough)
{
    POSITION xmid = (x2 - x1) / 2 + x1;
    POSITION ymid = (y2 - y1) / 2 + y1;
    if (x1 + 1 == x2) {
        return;
    }

    perturb_point_mid(floor_ptr, floor_ptr->grid_array[y1][x1].feat, floor_ptr->grid_array[y2][x1].feat, floor_ptr->grid_array[y1][x2].feat,
        floor_ptr->grid_array[y2][x2].feat, xmid, ymid, rough, depth_max);
    perturb_point_end(
        floor_ptr, floor_ptr->grid_array[y1][x1].feat, floor_ptr->grid_array[y1][x2].feat, floor_ptr->grid_array[ymid][xmid].feat, xmid, y1, rough, depth_max);
    perturb_point_end(
        floor_ptr, floor_ptr->grid_array[y1][x2].feat, floor_ptr->grid_array[y2][x2].feat, floor_ptr->grid_array[ymid][xmid].feat, x2, ymid, rough, depth_max);
    perturb_point_end(
        floor_ptr, floor_ptr->grid_array[y2][x2].feat, floor_ptr->grid_array[y2][x1].feat, floor_ptr->grid_array[ymid][xmid].feat, xmid, y2, rough, depth_max);
    perturb_point_end(
        floor_ptr, floor_ptr->grid_array[y2][x1].feat, floor_ptr->grid_array[y1][x1].feat, floor_ptr->grid_array[ymid][xmid].feat, x1, ymid, rough, depth_max);
    plasma_recursive(floor_ptr, x1, y1, xmid, ymid, depth_max, rough);
    plasma_recursive(floor_ptr, xmid, y1, x2, ymid, depth_max, rough);
    plasma_recursive(floor_ptr, x1, ymid, xmid, y2, depth_max, rough);
    plasma_recursive(floor_ptr, xmid, ymid, x2, y2, depth_max, rough);
}

/* The default table in terrain level generation. */
static int16_t terrain_table[MAX_WILDERNESS][MAX_FEAT_IN_TERRAIN];

/*!
 * @brief 荒野フロア生成のサブルーチン
 * @param terrain 荒野地形ID
 * @param seed 乱数の固定シード
 * @param border 未使用
 * @param corner 広域マップの角部分としての生成ならばTRUE
 */
static void generate_wilderness_area(FloorType *floor_ptr, int terrain, uint32_t seed, bool corner)
{
    if (terrain == TERRAIN_EDGE) {
        for (POSITION y1 = 0; y1 < MAX_HGT; y1++) {
            for (POSITION x1 = 0; x1 < MAX_WID; x1++) {
                floor_ptr->grid_array[y1][x1].feat = feat_permanent;
            }
        }

        return;
    }

    const auto state_backup = w_ptr->rng.get_state();
    w_ptr->rng.set_state(seed);
    int table_size = sizeof(terrain_table[0]) / sizeof(int16_t);
    if (!corner) {
        for (POSITION y1 = 0; y1 < MAX_HGT; y1++) {
            for (POSITION x1 = 0; x1 < MAX_WID; x1++) {
                floor_ptr->grid_array[y1][x1].feat = table_size / 2;
            }
        }
    }

    floor_ptr->grid_array[1][1].feat = (int16_t)randint0(table_size);
    floor_ptr->grid_array[MAX_HGT - 2][1].feat = (int16_t)randint0(table_size);
    floor_ptr->grid_array[1][MAX_WID - 2].feat = (int16_t)randint0(table_size);
    floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat = (int16_t)randint0(table_size);
    if (corner) {
        floor_ptr->grid_array[1][1].feat = terrain_table[terrain][floor_ptr->grid_array[1][1].feat];
        floor_ptr->grid_array[MAX_HGT - 2][1].feat = terrain_table[terrain][floor_ptr->grid_array[MAX_HGT - 2][1].feat];
        floor_ptr->grid_array[1][MAX_WID - 2].feat = terrain_table[terrain][floor_ptr->grid_array[1][MAX_WID - 2].feat];
        floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat = terrain_table[terrain][floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat];
        w_ptr->rng.set_state(state_backup);
        return;
    }

    int16_t north_west = floor_ptr->grid_array[1][1].feat;
    int16_t south_west = floor_ptr->grid_array[MAX_HGT - 2][1].feat;
    int16_t north_east = floor_ptr->grid_array[1][MAX_WID - 2].feat;
    int16_t south_east = floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat;
    FEAT_IDX roughness = 1; /* The roughness of the level. */
    plasma_recursive(floor_ptr, 1, 1, MAX_WID - 2, MAX_HGT - 2, table_size - 1, roughness);
    floor_ptr->grid_array[1][1].feat = north_west;
    floor_ptr->grid_array[MAX_HGT - 2][1].feat = south_west;
    floor_ptr->grid_array[1][MAX_WID - 2].feat = north_east;
    floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat = south_east;
    for (POSITION y1 = 1; y1 < MAX_HGT - 1; y1++) {
        for (POSITION x1 = 1; x1 < MAX_WID - 1; x1++) {
            floor_ptr->grid_array[y1][x1].feat = terrain_table[terrain][floor_ptr->grid_array[y1][x1].feat];
        }
    }

    w_ptr->rng.set_state(state_backup);
}

/*!
 * @brief 荒野フロア生成のメインルーチン
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 広域Y座標
 * @param x 広域X座標
 * @param is_border 広域マップの辺部分としての生成ならばTRUE
 * @param is_corner 広域マップの角部分としての生成ならばTRUE
 */
static void generate_area(PlayerType *player_ptr, POSITION y, POSITION x, bool is_border, bool is_corner)
{
    player_ptr->town_num = wilderness[y][x].town;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->base_level = wilderness[y][x].level;
    floor_ptr->dun_level = 0;
    floor_ptr->monster_level = floor_ptr->base_level;
    floor_ptr->object_level = floor_ptr->base_level;
    if (player_ptr->town_num) {
        init_buildings();
        if (is_border || is_corner) {
            init_flags = i2enum<init_flags_type>(INIT_CREATE_DUNGEON | INIT_ONLY_FEATURES);
        } else {
            init_flags = INIT_CREATE_DUNGEON;
        }

        parse_fixed_map(player_ptr, TOWN_DEFINITION_LIST, 0, 0, MAX_HGT, MAX_WID);
        if (!is_corner && !is_border) {
            player_ptr->visit |= (1UL << (player_ptr->town_num - 1));
        }
    } else {
        int terrain = wilderness[y][x].terrain;
        uint32_t seed = wilderness[y][x].seed;
        generate_wilderness_area(floor_ptr, terrain, seed, is_corner);
    }

    if (!is_corner && !wilderness[y][x].town) {
        //!< @todo make the road a bit more interresting.
        if (wilderness[y][x].road) {
            floor_ptr->grid_array[MAX_HGT / 2][MAX_WID / 2].feat = feat_floor;
            POSITION x1, y1;
            if (wilderness[y - 1][x].road) {
                /* North road */
                for (y1 = 1; y1 < MAX_HGT / 2; y1++) {
                    x1 = MAX_WID / 2;
                    floor_ptr->grid_array[y1][x1].feat = feat_floor;
                }
            }

            if (wilderness[y + 1][x].road) {
                /* North road */
                for (y1 = MAX_HGT / 2; y1 < MAX_HGT - 1; y1++) {
                    x1 = MAX_WID / 2;
                    floor_ptr->grid_array[y1][x1].feat = feat_floor;
                }
            }

            if (wilderness[y][x + 1].road) {
                /* East road */
                for (x1 = MAX_WID / 2; x1 < MAX_WID - 1; x1++) {
                    y1 = MAX_HGT / 2;
                    floor_ptr->grid_array[y1][x1].feat = feat_floor;
                }
            }

            if (wilderness[y][x - 1].road) {
                /* West road */
                for (x1 = 1; x1 < MAX_WID / 2; x1++) {
                    y1 = MAX_HGT / 2;
                    floor_ptr->grid_array[y1][x1].feat = feat_floor;
                }
            }
        }
    }

    bool is_winner = wilderness[y][x].entrance > 0;
    is_winner &= (wilderness[y][x].town == 0);
    bool is_wild_winner = dungeons_info[wilderness[y][x].entrance].flags.has_not(DungeonFeatureType::WINNER);
    is_winner &= ((w_ptr->total_winner != 0) || is_wild_winner);
    if (!is_winner) {
        return;
    }

    const auto state_backup = w_ptr->rng.get_state();
    w_ptr->rng.set_state(wilderness[y][x].seed);
    int dy = rand_range(6, floor_ptr->height - 6);
    int dx = rand_range(6, floor_ptr->width - 6);
    floor_ptr->grid_array[dy][dx].feat = feat_entrance;
    floor_ptr->grid_array[dy][dx].special = wilderness[y][x].entrance;
    w_ptr->rng.set_state(state_backup);
}

/*!
 * @brief 地上マップにモンスターを生成する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details '>' キーで普通に入った時と、襲撃を受けた時でモンスター数は異なる.
 * また、集団生成や護衛は、最初に生成された1体だけがカウント対象である.
 * よって、実際に生成されるモンスターは、コードの見た目より多くなる.
 */
static void generate_wild_monsters(PlayerType *player_ptr)
{
    constexpr auto num_ambush_monsters = 40;
    constexpr auto num_normal_monsters = 8;
    const auto lim = generate_encounter ? num_ambush_monsters : num_normal_monsters;
    for (auto i = 0; i < lim; i++) {
        BIT_FLAGS mode = 0;
        if (!(generate_encounter || (one_in_(2) && (!player_ptr->town_num)))) {
            mode |= PM_ALLOW_SLEEP;
        }

        (void)alloc_monster(player_ptr, generate_encounter ? 0 : 3, mode, summon_specific);
    }
}

/*!
 * @brief 広域マップの生成 /
 * Build the wilderness area outside of the town.
 * @todo 広域マップは恒常生成にする予定、PlayerTypeによる処理分岐は最終的に排除する。
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void wilderness_gen(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    floor_ptr->height = MAX_HGT;
    floor_ptr->width = MAX_WID;
    panel_row_min = floor_ptr->height;
    panel_col_min = floor_ptr->width;
    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, w_ptr->max_wild_y, w_ptr->max_wild_x);
    POSITION x = player_ptr->wilderness_x;
    POSITION y = player_ptr->wilderness_y;
    get_mon_num_prep(player_ptr, get_monster_hook(player_ptr), nullptr);

    /* North border */
    generate_area(player_ptr, y - 1, x, true, false);
    for (int i = 1; i < MAX_WID - 1; i++) {
        border.north[i] = floor_ptr->grid_array[MAX_HGT - 2][i].feat;
    }

    /* South border */
    generate_area(player_ptr, y + 1, x, true, false);
    for (int i = 1; i < MAX_WID - 1; i++) {
        border.south[i] = floor_ptr->grid_array[1][i].feat;
    }

    /* West border */
    generate_area(player_ptr, y, x - 1, true, false);
    for (int i = 1; i < MAX_HGT - 1; i++) {
        border.west[i] = floor_ptr->grid_array[i][MAX_WID - 2].feat;
    }

    /* East border */
    generate_area(player_ptr, y, x + 1, true, false);
    for (int i = 1; i < MAX_HGT - 1; i++) {
        border.east[i] = floor_ptr->grid_array[i][1].feat;
    }

    /* North west corner */
    generate_area(player_ptr, y - 1, x - 1, false, true);
    border.north_west = floor_ptr->grid_array[MAX_HGT - 2][MAX_WID - 2].feat;

    /* North east corner */
    generate_area(player_ptr, y - 1, x + 1, false, true);
    border.north_east = floor_ptr->grid_array[MAX_HGT - 2][1].feat;

    /* South west corner */
    generate_area(player_ptr, y + 1, x - 1, false, true);
    border.south_west = floor_ptr->grid_array[1][MAX_WID - 2].feat;

    /* South east corner */
    generate_area(player_ptr, y + 1, x + 1, false, true);
    border.south_east = floor_ptr->grid_array[1][1].feat;

    /* Create terrain of the current area */
    generate_area(player_ptr, y, x, false, false);

    /* Special boundary walls -- North */
    for (int i = 0; i < MAX_WID; i++) {
        floor_ptr->grid_array[0][i].feat = feat_permanent;
        floor_ptr->grid_array[0][i].mimic = border.north[i];
    }

    /* Special boundary walls -- South */
    for (int i = 0; i < MAX_WID; i++) {
        floor_ptr->grid_array[MAX_HGT - 1][i].feat = feat_permanent;
        floor_ptr->grid_array[MAX_HGT - 1][i].mimic = border.south[i];
    }

    /* Special boundary walls -- West */
    for (int i = 0; i < MAX_HGT; i++) {
        floor_ptr->grid_array[i][0].feat = feat_permanent;
        floor_ptr->grid_array[i][0].mimic = border.west[i];
    }

    /* Special boundary walls -- East */
    for (int i = 0; i < MAX_HGT; i++) {
        floor_ptr->grid_array[i][MAX_WID - 1].feat = feat_permanent;
        floor_ptr->grid_array[i][MAX_WID - 1].mimic = border.east[i];
    }

    floor_ptr->grid_array[0][0].mimic = border.north_west;
    floor_ptr->grid_array[0][MAX_WID - 1].mimic = border.north_east;
    floor_ptr->grid_array[MAX_HGT - 1][0].mimic = border.south_west;
    floor_ptr->grid_array[MAX_HGT - 1][MAX_WID - 1].mimic = border.south_east;
    for (y = 0; y < floor_ptr->height; y++) {
        for (x = 0; x < floor_ptr->width; x++) {
            grid_type *g_ptr;
            g_ptr = &floor_ptr->grid_array[y][x];
            if (is_daytime()) {
                g_ptr->info |= CAVE_GLOW;
                if (view_perma_grids) {
                    g_ptr->info |= CAVE_MARK;
                }

                continue;
            }

            TerrainType *f_ptr;
            f_ptr = &terrains_info[g_ptr->get_feat_mimic()];
            auto can_darken = !g_ptr->is_mirror();
            can_darken &= f_ptr->flags.has_none_of({ TerrainCharacteristics::QUEST_ENTER, TerrainCharacteristics::ENTRANCE });
            if (can_darken) {
                g_ptr->info &= ~(CAVE_GLOW);
                if (f_ptr->flags.has_not(TerrainCharacteristics::REMEMBER)) {
                    g_ptr->info &= ~(CAVE_MARK);
                }

                continue;
            }

            if (f_ptr->flags.has_not(TerrainCharacteristics::ENTRANCE)) {
                continue;
            }

            g_ptr->info |= CAVE_GLOW;
            if (view_perma_grids) {
                g_ptr->info |= CAVE_MARK;
            }
        }
    }

    if (player_ptr->teleport_town) {
        for (y = 0; y < floor_ptr->height; y++) {
            for (x = 0; x < floor_ptr->width; x++) {
                grid_type *g_ptr;
                g_ptr = &floor_ptr->grid_array[y][x];
                TerrainType *f_ptr;
                f_ptr = &terrains_info[g_ptr->feat];
                if (f_ptr->flags.has_not(TerrainCharacteristics::BLDG)) {
                    continue;
                }

                if ((f_ptr->subtype != 4) && !((player_ptr->town_num == 1) && (f_ptr->subtype == 0))) {
                    continue;
                }

                if (g_ptr->m_idx != 0) {
                    delete_monster_idx(player_ptr, g_ptr->m_idx);
                }

                player_ptr->oldpy = y;
                player_ptr->oldpx = x;
            }
        }

        player_ptr->teleport_town = false;
    } else if (player_ptr->leaving_dungeon) {
        for (y = 0; y < floor_ptr->height; y++) {
            for (x = 0; x < floor_ptr->width; x++) {
                grid_type *g_ptr;
                g_ptr = &floor_ptr->grid_array[y][x];
                if (!g_ptr->cave_has_flag(TerrainCharacteristics::ENTRANCE)) {
                    continue;
                }

                if (g_ptr->m_idx != 0) {
                    delete_monster_idx(player_ptr, g_ptr->m_idx);
                }

                player_ptr->oldpy = y;
                player_ptr->oldpx = x;
            }
        }

        player_ptr->teleport_town = false;
    }

    player_place(player_ptr, player_ptr->oldpy, player_ptr->oldpx);
    generate_wild_monsters(player_ptr);
    if (generate_encounter) {
        player_ptr->ambush_flag = true;
    }

    generate_encounter = false;
    set_floor_and_wall(0);
    auto &quest_list = QuestList::get_instance();
    for (auto &[q_idx, q_ref] : quest_list) {
        if (q_ref.status == QuestStatusType::REWARDED) {
            q_ref.status = QuestStatusType::FINISHED;
        }
    }
}

static int16_t conv_terrain2feat[MAX_WILDERNESS];

/*!
 * @brief 広域マップの生成(簡易処理版) /
 * Build the wilderness area. -DG-
 */
void wilderness_gen_small(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (int i = 0; i < MAX_WID; i++) {
        for (int j = 0; j < MAX_HGT; j++) {
            floor_ptr->grid_array[j][i].feat = feat_permanent;
        }
    }

    parse_fixed_map(player_ptr, WILDERNESS_DEFINITION, 0, 0, w_ptr->max_wild_y, w_ptr->max_wild_x);
    for (int i = 0; i < w_ptr->max_wild_x; i++) {
        for (int j = 0; j < w_ptr->max_wild_y; j++) {
            if (wilderness[j][i].town && (wilderness[j][i].town != VALID_TOWNS)) {
                floor_ptr->grid_array[j][i].feat = (int16_t)feat_town;
                floor_ptr->grid_array[j][i].special = (int16_t)wilderness[j][i].town;
                floor_ptr->grid_array[j][i].info |= (CAVE_GLOW | CAVE_MARK);
                continue;
            }

            if (wilderness[j][i].road) {
                floor_ptr->grid_array[j][i].feat = feat_floor;
                floor_ptr->grid_array[j][i].info |= (CAVE_GLOW | CAVE_MARK);
                continue;
            }

            if (wilderness[j][i].entrance && (w_ptr->total_winner || dungeons_info[wilderness[j][i].entrance].flags.has_not(DungeonFeatureType::WINNER))) {
                floor_ptr->grid_array[j][i].feat = feat_entrance;
                floor_ptr->grid_array[j][i].special = (byte)wilderness[j][i].entrance;
                floor_ptr->grid_array[j][i].info |= (CAVE_GLOW | CAVE_MARK);
                continue;
            }

            floor_ptr->grid_array[j][i].feat = conv_terrain2feat[wilderness[j][i].terrain];
            floor_ptr->grid_array[j][i].info |= (CAVE_GLOW | CAVE_MARK);
        }
    }

    floor_ptr->height = (int16_t)w_ptr->max_wild_y;
    floor_ptr->width = (int16_t)w_ptr->max_wild_x;
    if (floor_ptr->height > MAX_HGT) {
        floor_ptr->height = MAX_HGT;
    }

    if (floor_ptr->width > MAX_WID) {
        floor_ptr->width = MAX_WID;
    }

    panel_row_min = floor_ptr->height;
    panel_col_min = floor_ptr->width;
    player_ptr->x = player_ptr->wilderness_x;
    player_ptr->y = player_ptr->wilderness_y;
    player_ptr->town_num = 0;
}

struct wilderness_grid {
    wt_type terrain; /* Terrain type */
    int16_t town; /* Town number */
    DEPTH level; /* Level of the wilderness */
    byte road; /* Road */
    char name[32]; /* Name of the town/wilderness */
};

static wilderness_grid w_letter[255];

/*!
 * @brief w_info.txtのデータ解析 /
 * Parse a sub-file of the "extra info"
 * @param buf 読み取ったデータ行のバッファ
 * @param ymin 未使用
 * @param xmin 広域地形マップを読み込みたいx座標の開始位置
 * @param ymax 未使用
 * @param xmax 広域地形マップを読み込みたいx座標の終了位置
 * @param y 広域マップの高さを返す参照ポインタ
 * @param x 広域マップの幅を返す参照ポインタ
 */
parse_error_type parse_line_wilderness(PlayerType *player_ptr, char *buf, int xmin, int xmax, int *y, int *x)
{
    if (!(buf[0] == 'W')) {
        return PARSE_ERROR_GENERIC;
    }

    int num;
    char *zz[33];
    switch (buf[2]) {
        /* Process "W:F:<letter>:<terrain>:<town>:<road>:<name> */
#ifdef JP
    case 'E':
        return PARSE_ERROR_NONE;
    case 'F':
    case 'J':
#else
    case 'J':
        return PARSE_ERROR_NONE;
    case 'F':
    case 'E':
#endif
    {
        if ((num = tokenize(buf + 4, 6, zz, 0)) > 1) {
            int index = zz[0][0];

            if (num > 1) {
                w_letter[index].terrain = i2enum<wt_type>(atoi(zz[1]));
            } else {
                w_letter[index].terrain = TERRAIN_EDGE;
            }

            if (num > 2) {
                w_letter[index].level = (int16_t)atoi(zz[2]);
            } else {
                w_letter[index].level = 0;
            }

            if (num > 3) {
                w_letter[index].town = static_cast<int16_t>(atoi(zz[3]));
            } else {
                w_letter[index].town = 0;
            }

            if (num > 4) {
                w_letter[index].road = (byte)atoi(zz[4]);
            } else {
                w_letter[index].road = 0;
            }

            if (num > 5) {
                strcpy(w_letter[index].name, zz[5]);
            } else {
                w_letter[index].name[0] = 0;
            }
        } else {
            /* Failure */
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        break;
    }

    /* Process "W:D:<layout> */
    /* Layout of the wilderness */
    case 'D': {
        char *s = buf + 4;
        int len = strlen(s);
        int i;
        for (*x = xmin, i = 0; ((*x < xmax) && (i < len)); (*x)++, s++, i++) {
            int id = s[0];
            wilderness[*y][*x].terrain = w_letter[id].terrain;
            wilderness[*y][*x].level = w_letter[id].level;
            wilderness[*y][*x].town = w_letter[id].town;
            wilderness[*y][*x].road = w_letter[id].road;
            strcpy(town_info[w_letter[id].town].name, w_letter[id].name);
        }

        (*y)++;
        break;
    }

    /* Process "W:P:<x>:<y> - starting position in the wilderness */
    case 'P': {
        bool is_corner = player_ptr->wilderness_x == 0;
        is_corner = player_ptr->wilderness_y == 0;
        if (!is_corner) {
            break;
        }

        if (tokenize(buf + 4, 2, zz, 0) != 2) {
            return PARSE_ERROR_TOO_FEW_ARGUMENTS;
        }

        player_ptr->wilderness_y = atoi(zz[0]);
        player_ptr->wilderness_x = atoi(zz[1]);

        auto out_of_bounds = (player_ptr->wilderness_x < 1);
        out_of_bounds |= (player_ptr->wilderness_x > w_ptr->max_wild_x);
        out_of_bounds |= (player_ptr->wilderness_y < 1);
        out_of_bounds |= (player_ptr->wilderness_y > w_ptr->max_wild_y);
        if (out_of_bounds) {
            return PARSE_ERROR_OUT_OF_BOUNDS;
        }

        break;
    }

    default:
        return PARSE_ERROR_UNDEFINED_DIRECTIVE;
    }

    for (const auto &d_ref : dungeons_info) {
        if (d_ref.idx == 0 || !d_ref.maxdepth) {
            continue;
        }
        wilderness[d_ref.dy][d_ref.dx].entrance = static_cast<byte>(d_ref.idx);
        if (!wilderness[d_ref.dy][d_ref.dx].town) {
            wilderness[d_ref.dy][d_ref.dx].level = d_ref.mindepth;
        }
    }

    return PARSE_ERROR_NONE;
}

/*!
 * @brief ゲーム開始時に各荒野フロアの乱数シードを指定する /
 * Generate the random seeds for the wilderness
 */
void seed_wilderness(void)
{
    for (POSITION x = 0; x < w_ptr->max_wild_x; x++) {
        for (POSITION y = 0; y < w_ptr->max_wild_y; y++) {
            wilderness[y][x].seed = randint0(0x10000000);
            wilderness[y][x].entrance = 0;
        }
    }
}

/* Pointer to wilderness_type */
typedef wilderness_type *wilderness_type_ptr;

/*!
 * @brief ゲーム開始時の荒野初期化メインルーチン /
 * Initialize wilderness array
 * @return エラーコード
 */
errr init_wilderness(void)
{
    wilderness.assign(w_ptr->max_wild_y, std::vector<wilderness_type>(w_ptr->max_wild_x));

    generate_encounter = false;
    return 0;
}

/*!
 * @brief 荒野の地勢設定を初期化する /
 * Initialize wilderness array
 * @param terrain 初期化したい地勢ID
 * @param feat_global 基本的な地形ID
 * @param fmt 地勢内の地形数を参照するための独自フォーマット
 */
static void init_terrain_table(int terrain, int16_t feat_global, concptr fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    conv_terrain2feat[terrain] = feat_global;
    int cur = 0;
    char check = 'a';
    for (concptr p = fmt; *p; p++) {
        if (*p != check) {
            plog_fmt("Format error");
            continue;
        }

        FEAT_IDX feat = (int16_t)va_arg(vp, int);
        int num = va_arg(vp, int);
        int lim = cur + num;
        for (; (cur < lim) && (cur < MAX_FEAT_IN_TERRAIN); cur++) {
            terrain_table[terrain][cur] = feat;
        }

        if (cur >= MAX_FEAT_IN_TERRAIN) {
            break;
        }

        check++;
    }

    if (cur < MAX_FEAT_IN_TERRAIN) {
        plog_fmt("Too few parameters");
    }

    va_end(vp);
}

/*!
 * @brief 荒野の地勢設定全体を初期化するメインルーチン /
 * Initialize arrays for wilderness terrains
 */
void init_wilderness_terrains(void)
{
    init_terrain_table(TERRAIN_EDGE, feat_permanent, "a", feat_permanent, MAX_FEAT_IN_TERRAIN);
    init_terrain_table(TERRAIN_TOWN, feat_town, "a", feat_floor, MAX_FEAT_IN_TERRAIN);
    init_terrain_table(TERRAIN_DEEP_WATER, feat_deep_water, "ab", feat_deep_water, 12, feat_shallow_water, MAX_FEAT_IN_TERRAIN - 12);
    init_terrain_table(TERRAIN_SHALLOW_WATER, feat_shallow_water, "abcde", feat_deep_water, 3, feat_shallow_water, 12, feat_floor, 1, feat_dirt, 1, feat_grass,
        MAX_FEAT_IN_TERRAIN - 17);
    init_terrain_table(TERRAIN_SWAMP, feat_swamp, "abcdef", feat_dirt, 2, feat_grass, 3, feat_tree, 1, feat_brake, 1, feat_shallow_water, 4, feat_swamp,
        MAX_FEAT_IN_TERRAIN - 11);
    init_terrain_table(
        TERRAIN_DIRT, feat_dirt, "abcdef", feat_floor, 3, feat_dirt, 10, feat_flower, 1, feat_brake, 1, feat_grass, 1, feat_tree, MAX_FEAT_IN_TERRAIN - 16);
    init_terrain_table(
        TERRAIN_GRASS, feat_grass, "abcdef", feat_floor, 2, feat_dirt, 2, feat_grass, 9, feat_flower, 1, feat_brake, 2, feat_tree, MAX_FEAT_IN_TERRAIN - 16);
    init_terrain_table(TERRAIN_TREES, feat_tree, "abcde", feat_floor, 2, feat_dirt, 1, feat_tree, 11, feat_brake, 2, feat_grass, MAX_FEAT_IN_TERRAIN - 16);
    init_terrain_table(TERRAIN_DESERT, feat_dirt, "abc", feat_floor, 2, feat_dirt, 13, feat_grass, MAX_FEAT_IN_TERRAIN - 15);
    init_terrain_table(TERRAIN_SHALLOW_LAVA, feat_shallow_lava, "abc", feat_shallow_lava, 14, feat_deep_lava, 3, feat_mountain, MAX_FEAT_IN_TERRAIN - 17);
    init_terrain_table(
        TERRAIN_DEEP_LAVA, feat_deep_lava, "abcd", feat_dirt, 3, feat_shallow_lava, 3, feat_deep_lava, 10, feat_mountain, MAX_FEAT_IN_TERRAIN - 16);
    init_terrain_table(TERRAIN_MOUNTAIN, feat_mountain, "abcdef", feat_floor, 1, feat_brake, 1, feat_grass, 2, feat_dirt, 2, feat_tree, 2, feat_mountain,
        MAX_FEAT_IN_TERRAIN - 8);
}

/*!
 * @brief 荒野から広域マップへの切り替え処理 /
 * Initialize arrays for wilderness terrains
 * @param encount 襲撃時TRUE
 * @return 切り替えが行われた場合はTRUEを返す。
 */
bool change_wild_mode(PlayerType *player_ptr, bool encount)
{
    generate_encounter = encount;
    if (player_ptr->leaving) {
        return false;
    }

    if (lite_town || vanilla_town) {
        msg_print(_("荒野なんてない。", "No global map."));
        return false;
    }

    if (player_ptr->wild_mode) {
        player_ptr->wilderness_x = player_ptr->x;
        player_ptr->wilderness_y = player_ptr->y;
        player_ptr->energy_need = 0;
        player_ptr->wild_mode = false;
        player_ptr->leaving = true;
        return true;
    }

    bool has_pet = false;
    PlayerEnergy energy(player_ptr);
    for (int i = 1; i < player_ptr->current_floor_ptr->m_max; i++) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[i];
        if (!m_ptr->is_valid()) {
            continue;
        }

        if (m_ptr->is_pet() && i != player_ptr->riding) {
            has_pet = true;
        }

        if (m_ptr->is_asleep() || (m_ptr->cdis > MAX_PLAYER_SIGHT) || !m_ptr->is_hostile()) {
            continue;
        }

        msg_print(_("敵がすぐ近くにいるときは広域マップに入れない！", "You cannot enter global map, since there are some monsters nearby!"));
        energy.reset_player_turn();
        return false;
    }

    if (has_pet) {
        concptr msg = _("ペットを置いて広域マップに入りますか？", "Do you leave your pets behind? ");
        if (!get_check_strict(player_ptr, msg, CHECK_OKAY_CANCEL)) {
            energy.reset_player_turn();
            return false;
        }
    }

    energy.set_player_turn_energy(1000);
    player_ptr->oldpx = player_ptr->x;
    player_ptr->oldpy = player_ptr->y;
    SpellHex spell_hex(player_ptr);
    if (spell_hex.is_spelling_any()) {
        spell_hex.stop_all_spells();
    }

    set_action(player_ptr, ACTION_NONE);
    player_ptr->wild_mode = true;
    player_ptr->leaving = true;
    return true;
}
