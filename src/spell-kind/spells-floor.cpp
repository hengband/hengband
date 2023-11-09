/*!
 * @brief フロアに影響のある魔法の処理
 * @date 2019/02/21
 * @author deskull
 */

#include "spell-kind/spells-floor.h"
#include "action/travel-execution.h"
#include "cmd-io/cmd-dump.h"
#include "core/window-redrawer.h"
#include "dungeon/dungeon-flag-types.h"
#include "dungeon/quest.h"
#include "effect/attribute-types.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-save.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "grid/feature-flag-types.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "object-enchant/special-object-flags.h"
#include "object/object-mark-types.h"
#include "perception/object-perception.h"
#include "player/player-status-flags.h"
#include "player/special-defense-types.h"
#include "spell-kind/spells-teleport.h"
#include "status/bad-status-setter.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*
 * @brief 啓蒙/陽光召喚処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param ninja 忍者かどうか
 */
void wiz_lite(PlayerType *player_ptr, bool ninja)
{
    /* Memorize objects */
    auto &floor = *player_ptr->current_floor_ptr;
    for (OBJECT_IDX i = 1; i < floor.o_max; i++) {
        auto *o_ptr = &floor.o_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }
        if (o_ptr->is_held_by_monster()) {
            continue;
        }
        o_ptr->marked.set(OmType::FOUND);
    }

    /* Scan all normal grids */
    const auto &terrains = TerrainList::get_instance();
    for (POSITION y = 1; y < floor.height - 1; y++) {
        /* Scan all normal grids */
        for (POSITION x = 1; x < floor.width - 1; x++) {
            auto *g_ptr = &floor.grid_array[y][x];

            /* Memorize terrain of the grid */
            g_ptr->info |= (CAVE_KNOWN);

            /* Feature code (applying "mimic" field) */
            FEAT_IDX feat = g_ptr->get_feat_mimic();
            auto *t_ptr = &terrains[feat];

            /* Scan all neighbors */
            for (OBJECT_IDX i = 0; i < 9; i++) {
                POSITION yy = y + ddy_ddd[i];
                POSITION xx = x + ddx_ddd[i];
                g_ptr = &floor.grid_array[yy][xx];

                /* Feature code (applying "mimic" field) */
                t_ptr = &terrains[g_ptr->get_feat_mimic()];

                /* Perma-lite the grid */
                if (floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS) && !ninja) {
                    g_ptr->info |= (CAVE_GLOW);
                }

                /* Memorize normal features */
                if (t_ptr->flags.has(TerrainCharacteristics::REMEMBER)) {
                    /* Memorize the grid */
                    g_ptr->info |= (CAVE_MARK);
                }

                /* Perma-lit grids (newly and previously) */
                else if (g_ptr->info & CAVE_GLOW) {
                    /* Normally, memorize floors (see above) */
                    if (view_perma_grids && !view_torch_grids) {
                        /* Memorize the grid */
                        g_ptr->info |= (CAVE_MARK);
                    }
                }
            }
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
    if (floor.grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }
}

/*
 * Forget the dungeon map (ala "Thinking of Maud...").
 */
void wiz_dark(PlayerType *player_ptr)
{
    /* Forget every grid */
    for (POSITION y = 1; y < player_ptr->current_floor_ptr->height - 1; y++) {
        for (POSITION x = 1; x < player_ptr->current_floor_ptr->width - 1; x++) {
            auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[y][x];

            /* Process the grid */
            g_ptr->info &= ~(CAVE_MARK | CAVE_IN_DETECT | CAVE_KNOWN);
            g_ptr->info |= (CAVE_UNSAFE);
        }
    }

    /* Forget every grid on horizontal edge */
    for (POSITION x = 0; x < player_ptr->current_floor_ptr->width; x++) {
        player_ptr->current_floor_ptr->grid_array[0][x].info &= ~(CAVE_MARK);
        player_ptr->current_floor_ptr->grid_array[player_ptr->current_floor_ptr->height - 1][x].info &= ~(CAVE_MARK);
    }

    /* Forget every grid on vertical edge */
    for (POSITION y = 1; y < (player_ptr->current_floor_ptr->height - 1); y++) {
        player_ptr->current_floor_ptr->grid_array[y][0].info &= ~(CAVE_MARK);
        player_ptr->current_floor_ptr->grid_array[y][player_ptr->current_floor_ptr->width - 1].info &= ~(CAVE_MARK);
    }

    /* Forget all objects */
    for (OBJECT_IDX i = 1; i < player_ptr->current_floor_ptr->o_max; i++) {
        auto *o_ptr = &player_ptr->current_floor_ptr->o_list[i];

        if (!o_ptr->is_valid()) {
            continue;
        }
        if (o_ptr->is_held_by_monster()) {
            continue;
        }

        /* Forget the object */
        // 意図としては OmType::TOUCHED を維持しつつ OmType::FOUND を消す事と思われるが一応元のロジックを維持しておく
        o_ptr->marked &= { OmType::TOUCHED };
    }

    /* Forget travel route when we have forgotten map */
    forget_travel_flow(player_ptr->current_floor_ptr);

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
        SubWindowRedrawingFlag::FOUND_ITEMS,
    };
    rfu.set_flags(flags_swrf);
}

/*
 * Hack -- map the current panel (plus some) ala "magic mapping"
 */
void map_area(PlayerType *player_ptr, POSITION range)
{
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        range /= 3;
    }

    /* Scan that area */
    const auto &terrains = TerrainList::get_instance();
    for (POSITION y = 1; y < floor.height - 1; y++) {
        for (POSITION x = 1; x < floor.width - 1; x++) {
            if (distance(player_ptr->y, player_ptr->x, y, x) > range) {
                continue;
            }

            Grid *g_ptr;
            g_ptr = &floor.grid_array[y][x];

            /* Memorize terrain of the grid */
            g_ptr->info |= (CAVE_KNOWN);

            /* Feature code (applying "mimic" field) */
            FEAT_IDX feat = g_ptr->get_feat_mimic();
            auto *t_ptr = &terrains[feat];

            /* Memorize normal features */
            if (t_ptr->flags.has(TerrainCharacteristics::REMEMBER)) {
                /* Memorize the object */
                g_ptr->info |= (CAVE_MARK);
            }

            /* Memorize known walls */
            for (int i = 0; i < 8; i++) {
                g_ptr = &floor.grid_array[y + ddy_ddd[i]][x + ddx_ddd[i]];

                /* Feature code (applying "mimic" field) */
                feat = g_ptr->get_feat_mimic();
                t_ptr = &terrains[feat];

                /* Memorize walls (etc) */
                if (t_ptr->flags.has(TerrainCharacteristics::REMEMBER)) {
                    /* Memorize the walls */
                    g_ptr->info |= (CAVE_MARK);
                }
            }
        }
    }

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
}

/*!
 * @brief *破壊*処理を行う / The spell of destruction
 * @param y1 破壊の中心Y座標
 * @param x1 破壊の中心X座標
 * @param r 破壊の半径
 * @param in_generate ダンジョンフロア生成中の処理ならばTRUE
 * @return 効力があった場合TRUEを返す
 * @details
 * <pre>
 * This spell "deletes" monsters (instead of "killing" them).
 *
 * Later we may use one function for both "destruction" and
 * "earthquake" by using the "full" to select "destruction".
 * </pre>
 */
bool destroy_area(PlayerType *player_ptr, const POSITION y1, const POSITION x1, POSITION r, bool in_generate)
{
    const Pos2D pos1(y1, x1);

    /* Prevent destruction of quest levels and town */
    auto &floor = *player_ptr->current_floor_ptr;
    if ((floor.is_in_quest() && QuestType::is_fixed(floor.quest_number)) || !floor.dun_level) {
        return false;
    }

    /* Lose monster light */
    if (!in_generate) {
        clear_mon_lite(&floor);
    }

    /* Big area of affect */
    auto flag = false;
    for (auto y = (y1 - r); y <= (y1 + r); y++) {
        for (auto x = (x1 - r); x <= (x1 + r); x++) {
            const Pos2D pos(y, x);
            if (!in_bounds(&floor, pos.y, pos.x)) {
                continue;
            }

            /* Extract the distance */
            auto k = distance(pos1.y, pos1.x, pos.y, pos.x);

            /* Stay in the circle of death */
            if (k > r) {
                continue;
            }

            auto &grid = floor.get_grid(pos);

            /* Lose room and vault */
            grid.info &= ~(CAVE_ROOM | CAVE_ICKY);

            /* Lose light and knowledge */
            grid.info &= ~(CAVE_MARK | CAVE_GLOW | CAVE_KNOWN);

            if (!in_generate) /* Normal */
            {
                /* Lose unsafety */
                grid.info &= ~(CAVE_UNSAFE);

                /* Hack -- Notice player affect */
                if (player_ptr->is_located_at(pos)) {
                    /* Hurt the player later */
                    flag = true;

                    /* Do not hurt this grid */
                    continue;
                }
            }

            /* Hack -- Skip the epicenter */
            if (pos == pos1) {
                continue;
            }

            if (grid.m_idx) {
                auto &monster = floor.m_list[grid.m_idx];
                auto &monrace = monster.get_monrace();

                if (in_generate) /* In generation */
                {
                    /* Delete the monster (if any) */
                    delete_monster(player_ptr, pos.y, pos.x);
                } else if (monrace.flags1 & RF1_QUESTOR) {
                    /* Heal the monster */
                    monster.hp = monster.maxhp;

                    /* Try to teleport away quest monsters */
                    if (!teleport_away(player_ptr, grid.m_idx, (r * 2) + 1, TELEPORT_DEC_VALOUR)) {
                        continue;
                    }
                } else {
                    if (record_named_pet && monster.is_named_pet()) {
                        const auto m_name = monster_desc(player_ptr, &monster, MD_INDEF_VISIBLE);
                        exe_write_diary(player_ptr, DiaryKind::NAMED_PET, RECORD_NAMED_PET_DESTROY, m_name);
                    }

                    /* Delete the monster (if any) */
                    delete_monster(player_ptr, pos.y, pos.x);
                }
            }

            /* During generation, destroyed artifacts are "preserved" */
            if (preserve_mode || in_generate) {
                /* Scan all objects in the grid */
                for (const auto this_o_idx : grid.o_idx_list) {
                    const auto &item = floor.o_list[this_o_idx];
                    if (item.is_fixed_artifact() && (!item.is_known() || in_generate)) {
                        item.get_fixed_artifact().is_generated = false;

                        if (in_generate && cheat_peek) {
                            const auto item_name = describe_flavor(player_ptr, &item, (OD_NAME_ONLY | OD_STORE));
                            msg_format(_("伝説のアイテム (%s) は生成中に*破壊*された。", "Artifact (%s) was *destroyed* during generation."), item_name.data());
                        }
                    } else if (in_generate && cheat_peek && item.is_random_artifact()) {
                        msg_print(
                            _("ランダム・アーティファクトの1つは生成中に*破壊*された。", "One of the random artifacts was *destroyed* during generation."));
                    }
                }
            }

            delete_all_items_from_floor(player_ptr, pos.y, pos.x);

            /* Destroy "non-permanent" grids */
            if (grid.cave_has_flag(TerrainCharacteristics::PERMANENT)) {
                continue;
            }

            /* Wall (or floor) type */
            int t = randint0(200);

            if (!in_generate) /* Normal */
            {
                if (t < 20) {
                    /* Create granite wall */
                    cave_set_feat(player_ptr, pos.y, pos.x, feat_granite);
                } else if (t < 70) {
                    /* Create quartz vein */
                    cave_set_feat(player_ptr, pos.y, pos.x, feat_quartz_vein);
                } else if (t < 100) {
                    /* Create magma vein */
                    cave_set_feat(player_ptr, pos.y, pos.x, feat_magma_vein);
                } else {
                    /* Create floor */
                    cave_set_feat(player_ptr, pos.y, pos.x, rand_choice(feat_ground_type));
                }

                continue;
            }

            if (t < 20) {
                /* Create granite wall */
                place_grid(player_ptr, &grid, GB_EXTRA);
            } else if (t < 70) {
                /* Create quartz vein */
                grid.feat = feat_quartz_vein;
            } else if (t < 100) {
                /* Create magma vein */
                grid.feat = feat_magma_vein;
            } else {
                /* Create floor */
                place_grid(player_ptr, &grid, GB_FLOOR);
            }

            /* Clear garbage of hidden trap or door */
            grid.mimic = 0;
        }
    }

    if (in_generate) {
        return true;
    }

    /* Process "re-glowing" */
    for (auto y = (y1 - r); y <= (y1 + r); y++) {
        for (auto x = (x1 - r); x <= (x1 + r); x++) {
            const Pos2D pos(y, x);
            if (!in_bounds(&floor, pos.y, pos.x)) {
                continue;
            }

            /* Stay in the circle of death */
            auto k = distance(y1, x1, pos.y, pos.x);
            if (k > r) {
                continue;
            }

            auto &grid = floor.get_grid(pos);
            if (grid.is_mirror()) {
                grid.info |= CAVE_GLOW;
                continue;
            }

            if (floor.get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
                continue;
            }

            for (auto i = 0; i < 9; i++) {
                const Pos2D pos_neighbor(pos.y + ddy_ddd[i], pos.x + ddx_ddd[i]);
                if (!in_bounds2(&floor, pos_neighbor.y, pos_neighbor.x)) {
                    continue;
                }

                const auto &grid_neighbor = floor.get_grid(pos_neighbor);
                if (grid_neighbor.get_terrain_mimic().flags.has(TerrainCharacteristics::GLOW)) {
                    grid.info |= CAVE_GLOW;
                    break;
                }
            }
        }
    }

    if (flag) {
        msg_print(_("燃えるような閃光が発生した！", "There is a searing blast of light!"));
        if (!has_resist_blind(player_ptr) && !has_resist_lite(player_ptr)) {
            (void)BadStatusSetter(player_ptr).mod_blindness(10 + randint1(10));
        }
    }

    forget_flow(&floor);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::FLOW,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags_swrf);
    if (floor.grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }

    return true;
}
