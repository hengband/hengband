/*!
 * @brief ダンジョン生成に利用する関数群 / Used by dungeon generation.
 * @date 2014/07/15
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * @details
 * Purpose:  This file holds all the
 * functions that are applied to a level after the rest has been
 * generated, ie streams and level destruction.
 */

#include "floor/floor-streams.h"
#include "dungeon/dungeon-flag-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-generator-util.h"
#include "floor/floor-generator.h"
#include "floor/floor-object.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/cheat-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "room/lake-types.h"
#include "spell-kind/spells-floor.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon-data-definition.h"
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
#include "wizard/wizard-messages.h"

/*!
 * @brief 再帰フラクタルアルゴリズムによりダンジョン内に川を配置する /
 * Recursive fractal algorithm to place water through the dungeon.
 * @param x1 起点x座標
 * @param y1 起点y座標
 * @param x2 終点x座標
 * @param y2 終点y座標
 * @param feat1 中央部地形ID
 * @param feat2 境界部地形ID
 * @param width 基本幅
 */
static void recursive_river(FloorType *floor_ptr, POSITION x1, POSITION y1, POSITION x2, POSITION y2, FEAT_IDX feat1, FEAT_IDX feat2, POSITION width)
{
    POSITION dx, dy, length, l, x, y;
    POSITION changex, changey;
    POSITION ty, tx;
    bool done;
    grid_type *g_ptr;

    length = distance(x1, y1, x2, y2);

    if (length > 4) {
        /*
         * Divide path in half and call routine twice.
         * There is a small chance of splitting the river
         */
        dx = (x2 - x1) / 2;
        dy = (y2 - y1) / 2;

        if (dy != 0) {
            /* perturbation perpendicular to path */
            changex = randint1(abs(dy)) * 2 - abs(dy);
        } else {
            changex = 0;
        }

        if (dx != 0) {
            /* perturbation perpendicular to path */
            changey = randint1(abs(dx)) * 2 - abs(dx);
        } else {
            changey = 0;
        }

        if (!in_bounds(floor_ptr, y1 + dy + changey, x1 + dx + changex)) {
            changex = 0;
            changey = 0;
        }

        /* construct river out of two smaller ones */
        recursive_river(floor_ptr, x1, y1, x1 + dx + changex, y1 + dy + changey, feat1, feat2, width);
        recursive_river(floor_ptr, x1 + dx + changex, y1 + dy + changey, x2, y2, feat1, feat2, width);

        /* Split the river some of the time - junctions look cool */
        constexpr auto chance_river_junction = 50;
        if (one_in_(chance_river_junction) && (width > 0)) {
            recursive_river(floor_ptr, x1 + dx + changex, y1 + dy + changey, x1 + 8 * (dx + changex), y1 + 8 * (dy + changey), feat1, feat2, width - 1);
        }
    } else {
        /* Actually build the river */
        for (l = 0; l < length; l++) {
            x = x1 + l * (x2 - x1) / length;
            y = y1 + l * (y2 - y1) / length;

            done = false;

            while (!done) {
                for (ty = y - width - 1; ty <= y + width + 1; ty++) {
                    for (tx = x - width - 1; tx <= x + width + 1; tx++) {
                        if (!in_bounds2(floor_ptr, ty, tx)) {
                            continue;
                        }

                        g_ptr = &floor_ptr->grid_array[ty][tx];

                        if (g_ptr->feat == feat1) {
                            continue;
                        }
                        if (g_ptr->feat == feat2) {
                            continue;
                        }

                        if (distance(ty, tx, y, x) > rand_spread(width, 1)) {
                            continue;
                        }

                        /* Do not convert permanent features */
                        if (g_ptr->cave_has_flag(TerrainCharacteristics::PERMANENT)) {
                            continue;
                        }

                        /*
                         * Clear previous contents, add feature
                         * The border mainly gets feat2, while the center gets feat1
                         */
                        if (distance(ty, tx, y, x) > width) {
                            g_ptr->feat = feat2;
                        } else {
                            g_ptr->feat = feat1;
                        }

                        /* Clear garbage of hidden trap or door */
                        g_ptr->mimic = 0;

                        /* Lava terrain glows */
                        if (terrains_info[feat1].flags.has(TerrainCharacteristics::LAVA)) {
                            if (dungeons_info[floor_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS)) {
                                g_ptr->info |= CAVE_GLOW;
                            }
                        }

                        /* Hack -- don't teleport here */
                        g_ptr->info |= CAVE_ICKY;
                    }
                }

                done = true;
            }
        }
    }
}

/*!
 * @brief ランダムに川/溶岩流をダンジョンに配置する /
 * Places water /lava through dungeon.
 * @param feat1 中央部地形ID
 * @param feat2 境界部地形ID
 */
void add_river(FloorType *floor_ptr, dun_data_type *dd_ptr)
{
    dungeon_type *dungeon_ptr;
    POSITION y2, x2;
    POSITION y1 = 0, x1 = 0;
    POSITION wid;
    FEAT_IDX feat1 = 0, feat2 = 0;

    dungeon_ptr = &dungeons_info[floor_ptr->dungeon_idx];

    /* Choose water mainly */
    if ((randint1(MAX_DEPTH * 2) - 1 > floor_ptr->dun_level) && dungeon_ptr->flags.has(DungeonFeatureType::WATER_RIVER)) {
        feat1 = feat_deep_water;
        feat2 = feat_shallow_water;
    } else /* others */
    {
        FEAT_IDX select_deep_feat[10];
        FEAT_IDX select_shallow_feat[10];
        int select_id_max = 0, selected;

        if (dungeon_ptr->flags.has(DungeonFeatureType::LAVA_RIVER)) {
            select_deep_feat[select_id_max] = feat_deep_lava;
            select_shallow_feat[select_id_max] = feat_shallow_lava;
            select_id_max++;
        }
        if (dungeon_ptr->flags.has(DungeonFeatureType::POISONOUS_RIVER)) {
            select_deep_feat[select_id_max] = feat_deep_poisonous_puddle;
            select_shallow_feat[select_id_max] = feat_shallow_poisonous_puddle;
            select_id_max++;
        }
        if (dungeon_ptr->flags.has(DungeonFeatureType::ACID_RIVER)) {
            select_deep_feat[select_id_max] = feat_deep_acid_puddle;
            select_shallow_feat[select_id_max] = feat_shallow_acid_puddle;
            select_id_max++;
        }

        if (select_id_max > 0) {
            selected = randint0(select_id_max);
            feat1 = select_deep_feat[selected];
            feat2 = select_shallow_feat[selected];
        } else {
            return;
        }
    }

    if (feat1) {
        auto *f_ptr = &terrains_info[feat1];
        auto is_lava = dd_ptr->laketype == LAKE_T_LAVA;
        is_lava &= f_ptr->flags.has(TerrainCharacteristics::LAVA);
        auto is_water = dd_ptr->laketype == LAKE_T_WATER;
        is_water &= f_ptr->flags.has(TerrainCharacteristics::WATER);
        const auto should_add_river = !is_lava && !is_water && (dd_ptr->laketype != 0);
        if (should_add_river) {
            return;
        }
    }

    /* Hack -- Choose starting point */
    y2 = randint1(floor_ptr->height / 2 - 2) + floor_ptr->height / 2;
    x2 = randint1(floor_ptr->width / 2 - 2) + floor_ptr->width / 2;

    /* Hack -- Choose ending point somewhere on boundary */
    switch (randint1(4)) {
    case 1: {
        /* top boundary */
        x1 = randint1(floor_ptr->width - 2) + 1;
        y1 = 1;
        break;
    }
    case 2: {
        /* left boundary */
        x1 = 1;
        y1 = randint1(floor_ptr->height - 2) + 1;
        break;
    }
    case 3: {
        /* right boundary */
        x1 = floor_ptr->width - 1;
        y1 = randint1(floor_ptr->height - 2) + 1;
        break;
    }
    case 4: {
        /* bottom boundary */
        x1 = randint1(floor_ptr->width - 2) + 1;
        y1 = floor_ptr->height - 1;
        break;
    }
    }

    constexpr auto width_rivers = 2;
    wid = randint1(width_rivers);
    recursive_river(floor_ptr, x1, y1, x2, y2, feat1, feat2, wid);

    /* Hack - Save the location as a "room" */
    if (dd_ptr->cent_n < CENT_MAX) {
        dd_ptr->cent[dd_ptr->cent_n].y = y2;
        dd_ptr->cent[dd_ptr->cent_n].x = x2;
        dd_ptr->cent_n++;
    }
}

/*!
 * @brief ダンジョンの壁部にストリーマー（地質の変化）を与える /
 * Places "streamers" of rock through dungeon
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param feat ストリーマー地形ID
 * @param chance 生成密度
 * @details
 * <pre>
 * Note that their are actually six different terrain features used
 * to represent streamers.  Three each of magma and quartz, one for
 * basic vein, one with hidden gold, and one with known gold.  The
 * hidden gold types are currently unused.
 * </pre>
 */
void build_streamer(PlayerType *player_ptr, FEAT_IDX feat, int chance)
{
    int i;
    POSITION y, x, tx, ty;
    DIRECTION dir;
    int dummy = 0;

    grid_type *g_ptr;
    TerrainType *f_ptr;

    TerrainType *streamer_ptr = &terrains_info[feat];
    bool streamer_is_wall = streamer_ptr->flags.has(TerrainCharacteristics::WALL) && streamer_ptr->flags.has_not(TerrainCharacteristics::PERMANENT);
    bool streamer_may_have_gold = streamer_ptr->flags.has(TerrainCharacteristics::MAY_HAVE_GOLD);

    /* Hack -- Choose starting point */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    y = rand_spread(floor_ptr->height / 2, floor_ptr->height / 6);
    x = rand_spread(floor_ptr->width / 2, floor_ptr->width / 6);

    /* Choose a random compass direction */
    dir = randint0(8);

    /* Place streamer into dungeon */
    while (dummy < SAFE_MAX_ATTEMPTS) {
        dummy++;

        /* One grid per density */
        constexpr auto stream_density = 5;
        for (i = 0; i < stream_density; i++) {
            constexpr auto stream_width = 5;
            int d = stream_width;

            /* Pick a nearby grid */
            while (true) {
                ty = rand_spread(y, d);
                tx = rand_spread(x, d);
                if (!in_bounds2(floor_ptr, ty, tx)) {
                    continue;
                }
                break;
            }
            g_ptr = &floor_ptr->grid_array[ty][tx];
            f_ptr = &terrains_info[g_ptr->feat];

            if (f_ptr->flags.has(TerrainCharacteristics::MOVE) && f_ptr->flags.has_any_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::LAVA })) {
                continue;
            }

            /* Do not convert permanent features */
            if (f_ptr->flags.has(TerrainCharacteristics::PERMANENT)) {
                continue;
            }

            /* Only convert "granite" walls */
            if (streamer_is_wall) {
                if (!g_ptr->is_extra() && !g_ptr->is_inner() && !g_ptr->is_outer() && !g_ptr->is_solid()) {
                    continue;
                }
                if (is_closed_door(player_ptr, g_ptr->feat)) {
                    continue;
                }
            }

            auto *r_ptr = &monraces_info[floor_ptr->m_list[g_ptr->m_idx].r_idx];
            if (g_ptr->m_idx && !(streamer_ptr->flags.has(TerrainCharacteristics::PLACE) && monster_can_cross_terrain(player_ptr, feat, r_ptr, 0))) {
                /* Delete the monster (if any) */
                delete_monster(player_ptr, ty, tx);
            }

            if (!g_ptr->o_idx_list.empty() && streamer_ptr->flags.has_not(TerrainCharacteristics::DROP)) {

                /* Scan all objects in the grid */
                for (const auto this_o_idx : g_ptr->o_idx_list) {
                    auto *o_ptr = &floor_ptr->o_list[this_o_idx];

                    /* Hack -- Preserve unknown artifacts */
                    if (o_ptr->is_fixed_artifact()) {
                        ArtifactsInfo::get_instance().get_artifact(o_ptr->fixed_artifact_idx).is_generated = false;
                        if (cheat_peek) {
                            const auto item_name = describe_flavor(player_ptr, o_ptr, (OD_NAME_ONLY | OD_STORE));
                            msg_format(_("伝説のアイテム (%s) はストリーマーにより削除された。", "Artifact (%s) was deleted by streamer."), item_name.data());
                        }
                    } else if (cheat_peek && o_ptr->is_random_artifact()) {
                        msg_print(_("ランダム・アーティファクトの1つはストリーマーにより削除された。", "One of the random artifacts was deleted by streamer."));
                    }
                }

                delete_all_items_from_floor(player_ptr, ty, tx);
            }

            /* Clear previous contents, add proper vein type */
            g_ptr->feat = feat;

            /* Paranoia: Clear mimic field */
            g_ptr->mimic = 0;

            if (streamer_may_have_gold) {
                /* Hack -- Add some known treasure */
                if (one_in_(chance)) {
                    cave_alter_feat(player_ptr, ty, tx, TerrainCharacteristics::MAY_HAVE_GOLD);
                }

                /* Hack -- Add some hidden treasure */
                else if (one_in_(chance / 4)) {
                    cave_alter_feat(player_ptr, ty, tx, TerrainCharacteristics::MAY_HAVE_GOLD);
                    cave_alter_feat(player_ptr, ty, tx, TerrainCharacteristics::ENSECRET);
                }
            }
        }

        if (dummy >= SAFE_MAX_ATTEMPTS) {
            msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("地形のストリーマー処理に失敗しました。", "Failed to place streamer."));
            return;
        }

        /* Advance the streamer */
        y += ddy[cdd[dir]];
        x += ddx[cdd[dir]];

        if (one_in_(10)) {
            if (one_in_(2)) {
                dir = (dir + 1) % 8;
            } else {
                dir = (dir > 0) ? dir - 1 : 7;
            }
        }

        /* Quit before leaving the dungeon */
        if (!in_bounds(floor_ptr, y, x)) {
            break;
        }
    }
}

/*!
 * @brief ダンジョンの指定位置近辺に森林を配置する /
 * Places "streamers" of rock through dungeon
 * @param x 指定X座標
 * @param y 指定Y座標
 * @details
 * <pre>
 * Put trees near a hole in the dungeon roof  (rubble on ground + up stairway)
 * This happens in real world lava tubes.
 * </pre>
 */
void place_trees(PlayerType *player_ptr, POSITION x, POSITION y)
{
    int i, j;
    grid_type *g_ptr;

    /* place trees/ rubble in ovalish distribution */
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (i = x - 3; i < x + 4; i++) {
        for (j = y - 3; j < y + 4; j++) {
            if (!in_bounds(floor_ptr, j, i)) {
                continue;
            }
            g_ptr = &floor_ptr->grid_array[j][i];

            if (g_ptr->info & CAVE_ICKY) {
                continue;
            }
            if (!g_ptr->o_idx_list.empty()) {
                continue;
            }

            /* Want square to be in the circle and accessable. */
            if ((distance(j, i, y, x) < 4) && !g_ptr->cave_has_flag(TerrainCharacteristics::PERMANENT)) {
                /*
                 * Clear previous contents, add feature
                 * The border mainly gets trees, while the center gets rubble
                 */
                if ((distance(j, i, y, x) > 1) || (randint1(100) < 25)) {
                    if (randint1(100) < 75) {
                        floor_ptr->grid_array[j][i].feat = feat_tree;
                    }
                } else {
                    floor_ptr->grid_array[j][i].feat = feat_rubble;
                }

                /* Clear garbage of hidden trap or door */
                g_ptr->mimic = 0;

                /* Light area since is open above */
                if (dungeons_info[player_ptr->dungeon_idx].flags.has_not(DungeonFeatureType::DARKNESS)) {
                    floor_ptr->grid_array[j][i].info |= (CAVE_GLOW | CAVE_ROOM);
                }
            }
        }
    }

    /* No up stairs in ironman mode */
    if (!ironman_downward && one_in_(3)) {
        /* up stair */
        floor_ptr->grid_array[y][x].feat = feat_up_stair;
    }
}

/*!
 * @brief ダンジョンに＊破壊＊済み地形ランダムに施す /
 * Build a destroyed level
 */
void destroy_level(PlayerType *player_ptr)
{
    msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("階に*破壊*の痕跡を生成しました。", "Destroyed Level."));

    /* Drop a few epi-centers (usually about two) */
    POSITION y1, x1;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    for (int n = 0; n < randint1(5); n++) {
        /* Pick an epi-center */
        x1 = rand_range(5, floor_ptr->width - 1 - 5);
        y1 = rand_range(5, floor_ptr->height - 1 - 5);

        (void)destroy_area(player_ptr, y1, x1, 15, true);
    }
}
