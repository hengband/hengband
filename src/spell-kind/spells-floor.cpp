/*!
 * @brief フロアに影響のある魔法の処理
 * @date 2019/02/21
 * @author deskull
 */

#include "spell-kind/spells-floor.h"
#include "action/travel-execution.h"
#include "dungeon/quest.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-util.h"
#include "floor/geometry.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "io/write-diary.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "player/player-status-flags.h"
#include "spell-kind/spells-teleport.h"
#include "status/bad-status-setter.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
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
    for (auto y = 1; y < floor.height - 1; y++) {
        /* Scan all normal grids */
        for (auto x = 1; x < floor.width - 1; x++) {
            const Pos2D pos(y, x);
            auto &grid = floor.get_grid(pos);

            /* Memorize terrain of the grid */
            grid.info |= (CAVE_KNOWN);

            /* Scan all neighbors */
            for (const auto &d : Direction::directions()) {
                const auto pos_neighbor = pos + d.vec();
                auto &grid_neighbor = floor.get_grid(pos_neighbor);

                /* Feature code (applying "mimic" field) */
                const auto &terrain = terrains.get_terrain(grid_neighbor.get_terrain_id(TerrainKind::MIMIC));

                /* Perma-lite the grid */
                if (floor.get_dungeon_definition().flags.has_not(DungeonFeatureType::DARKNESS) && !ninja) {
                    grid_neighbor.info |= (CAVE_GLOW);
                }

                /* Memorize normal features */
                if (terrain.flags.has(TerrainCharacteristics::REMEMBER)) {
                    /* Memorize the grid */
                    grid_neighbor.info |= (CAVE_MARK);
                }

                /* Perma-lit grids (newly and previously) */
                else if (grid_neighbor.info & CAVE_GLOW) {
                    /* Normally, memorize floors (see above) */
                    if (view_perma_grids && !view_torch_grids) {
                        /* Memorize the grid */
                        grid_neighbor.info |= (CAVE_MARK);
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
            auto &grid = player_ptr->current_floor_ptr->grid_array[y][x];

            /* Process the grid */
            grid.info &= ~(CAVE_MARK | CAVE_IN_DETECT | CAVE_KNOWN);
            grid.info |= (CAVE_UNSAFE);
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
    forget_travel_flow(*player_ptr->current_floor_ptr);

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
    for (auto y = 1; y < floor.height - 1; y++) {
        for (auto x = 1; x < floor.width - 1; x++) {
            const Pos2D pos(y, x);
            if (Grid::calc_distance(player_ptr->get_position(), pos) > range) {
                continue;
            }

            auto &grid = floor.get_grid(pos);

            /* Memorize terrain of the grid */
            grid.info |= (CAVE_KNOWN);

            /* Feature code (applying "mimic" field) */
            const auto mimic_terrain_id = grid.get_terrain_id(TerrainKind::MIMIC);
            const auto &terrain_mimic = terrains.get_terrain(mimic_terrain_id);

            /* Memorize normal features */
            if (terrain_mimic.flags.has(TerrainCharacteristics::REMEMBER)) {
                /* Memorize the object */
                grid.info |= (CAVE_MARK);
            }

            /* Memorize known walls */
            for (const auto &d : Direction::directions_8()) {
                auto &grid_neighbor = floor.get_grid(pos + d.vec());

                /* Feature code (applying "mimic" field) */
                const auto terrain_id = grid_neighbor.get_terrain_id(TerrainKind::MIMIC);
                const auto &terrain = terrains.get_terrain(terrain_id);

                /* Memorize walls (etc) */
                if (terrain.flags.has(TerrainCharacteristics::REMEMBER)) {
                    /* Memorize the walls */
                    grid_neighbor.info |= (CAVE_MARK);
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
    if ((floor.is_in_quest() && QuestType::is_fixed(floor.quest_number)) || !floor.is_underground()) {
        return false;
    }

    /* Lose monster light */
    if (!in_generate) {
        clear_mon_lite(floor);
    }

    /* Big area of affect */
    const auto &dungeon = floor.get_dungeon_definition();
    auto flag = false;
    for (auto y = (y1 - r); y <= (y1 + r); y++) {
        for (auto x = (x1 - r); x <= (x1 + r); x++) {
            const Pos2D pos(y, x);
            if (!in_bounds(floor, pos.y, pos.x)) {
                continue;
            }

            /* Extract the distance */
            auto k = Grid::calc_distance(pos1, pos);

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

            if (grid.has_monster()) {
                auto &monster = floor.m_list[grid.m_idx];
                auto &monrace = monster.get_monrace();

                if (in_generate) /* In generation */
                {
                    /* Delete the monster (if any) */
                    delete_monster(player_ptr, pos);
                } else if (monrace.misc_flags.has(MonsterMiscType::QUESTOR)) {
                    /* Heal the monster */
                    monster.hp = monster.maxhp;

                    /* Try to teleport away quest monsters */
                    if (!teleport_away(player_ptr, grid.m_idx, (r * 2) + 1, TELEPORT_DEC_VALOUR)) {
                        continue;
                    }
                } else {
                    if (record_named_pet && monster.is_named_pet()) {
                        const auto m_name = monster_desc(player_ptr, monster, MD_INDEF_VISIBLE);
                        exe_write_diary(floor, DiaryKind::NAMED_PET, RECORD_NAMED_PET_DESTROY, m_name);
                    }

                    /* Delete the monster (if any) */
                    delete_monster(player_ptr, pos);
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
                            const auto item_name = describe_flavor(player_ptr, item, (OD_NAME_ONLY | OD_STORE));
                            msg_format(_("伝説のアイテム (%s) は生成中に*破壊*された。", "Artifact (%s) was *destroyed* during generation."), item_name.data());
                        }
                    } else if (in_generate && cheat_peek && item.is_random_artifact()) {
                        msg_print(
                            _("ランダム・アーティファクトの1つは生成中に*破壊*された。", "One of the random artifacts was *destroyed* during generation."));
                    }
                }
            }

            delete_all_items_from_floor(player_ptr, pos);

            /* Destroy "non-permanent" grids */
            if (grid.has(TerrainCharacteristics::PERMANENT)) {
                continue;
            }

            /* Wall (or floor) type */
            int t = randint0(200);

            if (!in_generate) /* Normal */
            {
                if (t < 20) {
                    /* Create granite wall */
                    set_terrain_id_to_grid(player_ptr, pos, TerrainTag::GRANITE_WALL);
                } else if (t < 70) {
                    /* Create quartz vein */
                    set_terrain_id_to_grid(player_ptr, pos, TerrainTag::QUARTZ_VEIN);
                } else if (t < 100) {
                    /* Create magma vein */
                    set_terrain_id_to_grid(player_ptr, pos, TerrainTag::MAGMA_VEIN);
                } else {
                    /* Create floor */
                    set_terrain_id_to_grid(player_ptr, pos, dungeon.select_floor_terrain_id());
                }

                continue;
            }

            if (t < 20) {
                /* Create granite wall */
                place_grid(player_ptr, grid, GB_EXTRA);
            } else if (t < 70) {
                /* Create quartz vein */
                grid.set_terrain_id(TerrainTag::QUARTZ_VEIN);
            } else if (t < 100) {
                /* Create magma vein */
                grid.set_terrain_id(TerrainTag::MAGMA_VEIN);
            } else {
                /* Create floor */
                place_grid(player_ptr, grid, GB_FLOOR);
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
            if (!in_bounds(floor, pos.y, pos.x)) {
                continue;
            }

            /* Stay in the circle of death */
            auto k = Grid::calc_distance(pos1, pos);
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

            for (const auto &d : Direction::directions()) {
                const auto pos_neighbor = pos + d.vec();
                if (!in_bounds2(floor, pos_neighbor.y, pos_neighbor.x)) {
                    continue;
                }

                const auto &grid_neighbor = floor.get_grid(pos_neighbor);
                if (grid_neighbor.get_terrain(TerrainKind::MIMIC).flags.has(TerrainCharacteristics::GLOW)) {
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

    forget_flow(floor);
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
