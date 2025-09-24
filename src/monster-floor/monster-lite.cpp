#include "monster-floor/monster-lite.h"
#include "dungeon/dungeon-flag-types.h"
#include "monster-floor/monster-lite-util.h"
#include "monster-race/race-brightness-flags.h"
#include "monster/monster-status.h"
#include "player-base/player-class.h"
#include "player-info/ninja-data-type.h"
#include "player/special-defense-types.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-characteristics.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/point-2d.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <vector>

/*!
 * @brief モンスターによる光量状態更新 (照らす方)
 * @param floor フロアへの参照
 * @param points 座標記録用の配列
 * @param p_pos プレイヤーの座標
 * @param pos 更新対象の座標
 * @param monster_lite モンスター光量状態への参照 (TODO: 後で消す)
 */
static void update_monster_lite(
    FloorType &floor, std::vector<Pos2D> &points, const Pos2D &p_pos, const Pos2D &pos, const monster_lite_type &monster_lite)
{
    auto &grid = floor.get_grid(pos);
    if ((grid.info & (CAVE_MNLT | CAVE_VIEW)) != CAVE_VIEW) {
        return;
    }

    if (!grid.has_los_terrain()) {
        if (((pos.y < p_pos.y) && (pos.y > monster_lite.m_pos.y)) || ((pos.y > p_pos.y) && (pos.y < monster_lite.m_pos.y))) {
            const auto dpf = p_pos.y - monster_lite.m_pos.y;
            const auto d = pos.y - monster_lite.m_pos.y;
            const auto midpoint = monster_lite.m_pos.x + ((p_pos.x - monster_lite.m_pos.x) * std::abs(d)) / std::abs(dpf);
            if (pos.x < midpoint) {
                if (!floor.has_los_terrain_at(pos + Direction(6).vec())) {
                    return;
                }
            } else if (pos.x > midpoint) {
                if (!floor.has_los_terrain_at(pos + Direction(4).vec())) {
                    return;
                }
            } else if (monster_lite.mon_invis) {
                return;
            }
        }

        if (((pos.x < p_pos.x) && (pos.x > monster_lite.m_pos.x)) || ((pos.x > p_pos.x) && (pos.x < monster_lite.m_pos.x))) {
            const auto dpf = p_pos.x - monster_lite.m_pos.x;
            const auto d = pos.x - monster_lite.m_pos.x;
            const auto midpoint = monster_lite.m_pos.y + ((p_pos.y - monster_lite.m_pos.y) * std::abs(d)) / std::abs(dpf);
            if (pos.y < midpoint) {
                if (!floor.has_los_terrain_at(pos + Direction(2).vec())) {
                    return;
                }
            } else if (pos.y > midpoint) {
                if (!floor.has_los_terrain_at(pos + Direction(8).vec())) {
                    return;
                }
            } else if (monster_lite.mon_invis) {
                return;
            }
        }
    }

    if (!(grid.info & CAVE_MNDK)) {
        points.push_back(pos);
    } else {
        grid.info &= ~(CAVE_MNDK);
    }

    grid.info |= CAVE_MNLT;
}

/*!
 * @brief モンスターによる光量状態更新 (暗くする方)
 * @param floor フロアへの参照
 * @param points 座標記録用の配列
 * @param p_pos プレイヤーの座標
 * @param pos 更新対象の座標
 * @param monster_lite モンスター光量状態への参照 (TODO: 後で消す)
 */
static void update_monster_dark(
    FloorType &floor, std::vector<Pos2D> &points, const Pos2D &p_pos, const Pos2D &pos, const monster_lite_type &monster_lite)
{
    auto &grid = floor.get_grid(pos);
    if ((grid.info & (CAVE_LITE | CAVE_MNLT | CAVE_MNDK | CAVE_VIEW)) != CAVE_VIEW) {
        return;
    }

    if (!grid.has_los_terrain() && !grid.has(TerrainCharacteristics::PROJECTION)) {
        if (((pos.y < p_pos.y) && (pos.y > monster_lite.m_pos.y)) || ((pos.y > p_pos.y) && (pos.y < monster_lite.m_pos.y))) {
            const auto dpf = p_pos.y - monster_lite.m_pos.y;
            const auto d = pos.y - monster_lite.m_pos.y;
            const auto midpoint = monster_lite.m_pos.x + ((p_pos.x - monster_lite.m_pos.x) * std::abs(d)) / std::abs(dpf);
            if (pos.x < midpoint) {
                if (!floor.has_los_terrain_at(pos + Direction(6).vec()) && !floor.has_terrain_characteristics({ pos.y, pos.x + 1 }, TerrainCharacteristics::PROJECTION)) {
                    return;
                }
            } else if (pos.x > midpoint) {
                if (!floor.has_los_terrain_at(pos + Direction(4).vec()) && !floor.has_terrain_characteristics({ pos.y, pos.x - 1 }, TerrainCharacteristics::PROJECTION)) {
                    return;
                }
            } else if (monster_lite.mon_invis) {
                return;
            }
        }

        if (((pos.x < p_pos.x) && (pos.x > monster_lite.m_pos.x)) || ((pos.x > p_pos.x) && (pos.x < monster_lite.m_pos.x))) {
            const auto dpf = p_pos.x - monster_lite.m_pos.x;
            const auto d = pos.x - monster_lite.m_pos.x;
            const auto midpoint = monster_lite.m_pos.y + ((p_pos.y - monster_lite.m_pos.y) * std::abs(d)) / std::abs(dpf);
            if (pos.y < midpoint) {
                if (!floor.has_los_terrain_at(pos + Direction(2).vec()) && !floor.has_terrain_characteristics({ pos.y + 1, pos.x }, TerrainCharacteristics::PROJECTION)) {
                    return;
                }
            } else if (pos.y > midpoint) {
                if (!floor.has_los_terrain_at(pos + Direction(8).vec()) && !floor.has_terrain_characteristics({ pos.y - 1, pos.x }, TerrainCharacteristics::PROJECTION)) {
                    return;
                }
            } else if (monster_lite.mon_invis) {
                return;
            }
        }
    }

    points.push_back(pos);
    grid.info |= CAVE_MNDK;
}

/*!
 * @brief Update squares illuminated or darkened by monsters.
 * The CAVE_TEMP and CAVE_XTRA flag are used to store the state during the
 * updating.  Only squares in view of the player, whos state
 * changes are drawn via lite_spot().
 * @todo player-status からのみ呼ばれている。しかしあちらは行数が酷いので要調整
 */
void update_mon_lite(PlayerType *player_ptr)
{
    // 座標たちを記録する配列。
    std::vector<Pos2D> points;

    void (*add_mon_lite)(FloorType &, std::vector<Pos2D> &, const Pos2D &p_pos, const Pos2D &pos, const monster_lite_type &);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    const auto dis_lim = (dungeon.flags.has(DungeonFeatureType::DARKNESS) && !player_ptr->see_nocto) ? (MAX_PLAYER_SIGHT / 2 + 1) : (MAX_PLAYER_SIGHT + 3);
    floor.reset_mon_lite();
    const auto &world = AngbandWorld::get_instance();
    const auto p_pos = player_ptr->get_position();
    if (!world.timewalk_m_idx) {
        for (auto i = 1; i < floor.m_max; i++) {
            const auto &monster = floor.m_list[i];
            const auto &monrace = monster.get_monrace();
            if (!monster.is_valid() || (monster.cdis > dis_lim)) {
                continue;
            }

            auto rad = 0;
            if (monrace.brightness_flags.has_any_of({ MonsterBrightnessType::HAS_LITE_1, MonsterBrightnessType::SELF_LITE_1 })) {
                rad++;
            }

            if (monrace.brightness_flags.has_any_of({ MonsterBrightnessType::HAS_LITE_2, MonsterBrightnessType::SELF_LITE_2 })) {
                rad += 2;
            }

            if (monrace.brightness_flags.has_any_of({ MonsterBrightnessType::HAS_DARK_1, MonsterBrightnessType::SELF_DARK_1 })) {
                rad--;
            }

            if (monrace.brightness_flags.has_any_of({ MonsterBrightnessType::HAS_DARK_2, MonsterBrightnessType::SELF_DARK_2 })) {
                rad -= 2;
            }

            if (!rad) {
                continue;
            }

            TerrainCharacteristics tc;
            if (rad > 0) {
                auto should_lite = monrace.brightness_flags.has_none_of({ MonsterBrightnessType::SELF_LITE_1, MonsterBrightnessType::SELF_LITE_2 });
                should_lite &= (monster.is_asleep() || (!floor.is_underground() && world.is_daytime()) || AngbandSystem::get_instance().is_phase_out());
                if (should_lite) {
                    continue;
                }

                if (dungeon.flags.has(DungeonFeatureType::DARKNESS)) {
                    rad = 1;
                }

                add_mon_lite = update_monster_lite;
                tc = TerrainCharacteristics::LOS;
            } else {
                if (monrace.brightness_flags.has_none_of({ MonsterBrightnessType::SELF_DARK_1, MonsterBrightnessType::SELF_DARK_2 }) && (monster.is_asleep() || (!floor.is_underground() && !world.is_daytime()))) {
                    continue;
                }

                add_mon_lite = update_monster_dark;
                tc = TerrainCharacteristics::PROJECTION;
                rad = -rad;
            }

            monster_lite_type monster_lite(floor.get_grid(monster.get_position()).info, monster);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y, monster_lite.m_pos.x }, monster_lite);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 1, monster_lite.m_pos.x }, monster_lite);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 1, monster_lite.m_pos.x }, monster_lite);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y, monster_lite.m_pos.x + 1 }, monster_lite);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y, monster_lite.m_pos.x - 1 }, monster_lite);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 1, monster_lite.m_pos.x + 1 }, monster_lite);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 1, monster_lite.m_pos.x - 1 }, monster_lite);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 1, monster_lite.m_pos.x + 1 }, monster_lite);
            add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 1, monster_lite.m_pos.x - 1 }, monster_lite);
            if (rad < 2) {
                continue;
            }

            if (floor.has_terrain_characteristics({ monster_lite.m_pos.y + 1, monster_lite.m_pos.x }, tc)) {
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 2, monster_lite.m_pos.x + 1 }, monster_lite);
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 2, monster_lite.m_pos.x }, monster_lite);
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 2, monster_lite.m_pos.x - 1 }, monster_lite);
                const auto &grid = floor.grid_array[monster_lite.m_pos.y + 2][monster_lite.m_pos.x];
                if ((rad == 3) && grid.has(tc)) {
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 3, monster_lite.m_pos.x + 1 }, monster_lite);
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 3, monster_lite.m_pos.x }, monster_lite);
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 3, monster_lite.m_pos.x - 1 }, monster_lite);
                }
            }

            if (floor.has_terrain_characteristics({ monster_lite.m_pos.y - 1, monster_lite.m_pos.x }, tc)) {
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 2, monster_lite.m_pos.x + 1 }, monster_lite);
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 2, monster_lite.m_pos.x }, monster_lite);
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 2, monster_lite.m_pos.x - 1 }, monster_lite);
                const auto &grid = floor.grid_array[monster_lite.m_pos.y - 2][monster_lite.m_pos.x];
                if ((rad == 3) && grid.has(tc)) {
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 3, monster_lite.m_pos.x + 1 }, monster_lite);
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 3, monster_lite.m_pos.x }, monster_lite);
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 3, monster_lite.m_pos.x - 1 }, monster_lite);
                }
            }

            if (floor.has_terrain_characteristics({ monster_lite.m_pos.y, monster_lite.m_pos.x + 1 }, tc)) {
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 1, monster_lite.m_pos.x + 2 }, monster_lite);
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y, monster_lite.m_pos.x + 2 }, monster_lite);
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 1, monster_lite.m_pos.x + 2 }, monster_lite);
                const auto &grid = floor.grid_array[monster_lite.m_pos.y][monster_lite.m_pos.x + 2];
                if ((rad == 3) && grid.has(tc)) {
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 1, monster_lite.m_pos.x + 3 }, monster_lite);
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y, monster_lite.m_pos.x + 3 }, monster_lite);
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 1, monster_lite.m_pos.x + 3 }, monster_lite);
                }
            }

            if (floor.has_terrain_characteristics({ monster_lite.m_pos.y, monster_lite.m_pos.x - 1 }, tc)) {
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 1, monster_lite.m_pos.x - 2 }, monster_lite);
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y, monster_lite.m_pos.x - 2 }, monster_lite);
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 1, monster_lite.m_pos.x - 2 }, monster_lite);
                const auto &grid = floor.grid_array[monster_lite.m_pos.y][monster_lite.m_pos.x - 2];
                if ((rad == 3) && grid.has(tc)) {
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 1, monster_lite.m_pos.x - 3 }, monster_lite);
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y, monster_lite.m_pos.x - 3 }, monster_lite);
                    add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 1, monster_lite.m_pos.x - 3 }, monster_lite);
                }
            }

            if (rad != 3) {
                continue;
            }

            if (floor.has_terrain_characteristics({ monster_lite.m_pos.y + 1, monster_lite.m_pos.x + 1 }, tc)) {
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 2, monster_lite.m_pos.x + 2 }, monster_lite);
            }

            if (floor.has_terrain_characteristics({ monster_lite.m_pos.y + 1, monster_lite.m_pos.x - 1 }, tc)) {
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y + 2, monster_lite.m_pos.x - 2 }, monster_lite);
            }

            if (floor.has_terrain_characteristics({ monster_lite.m_pos.y - 1, monster_lite.m_pos.x + 1 }, tc)) {
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 2, monster_lite.m_pos.x + 2 }, monster_lite);
            }

            if (floor.has_terrain_characteristics({ monster_lite.m_pos.y - 1, monster_lite.m_pos.x - 1 }, tc)) {
                add_mon_lite(floor, points, p_pos, { monster_lite.m_pos.y - 2, monster_lite.m_pos.x - 2 }, monster_lite);
            }
        }
    }

    const auto end_temp = std::size(points);
    for (auto i = 0; i < floor.mon_lite_n; i++) {
        const auto fx = floor.mon_lite_x[i];
        const auto fy = floor.mon_lite_y[i];
        const auto pos = Pos2D(fy, fx);
        const auto &grid = floor.get_grid(pos);
        if (grid.info & CAVE_TEMP) {
            if ((grid.info & (CAVE_VIEW | CAVE_MNLT)) == CAVE_VIEW) {
                floor.set_note_and_redraw_at(pos);
            }
        } else if ((grid.info & (CAVE_VIEW | CAVE_MNDK)) == CAVE_VIEW) {
            floor.set_note_and_redraw_at(pos);
        }

        points.push_back(pos);
    }

    floor.mon_lite_n = 0;
    for (size_t i = 0; i < end_temp; i++) {
        const auto &pos = points[i];
        const auto &grid = floor.get_grid(pos);
        if (grid.info & CAVE_MNLT) {
            if ((grid.info & (CAVE_VIEW | CAVE_TEMP)) == CAVE_VIEW) {
                floor.set_note_and_redraw_at(pos);
            }
        } else if ((grid.info & (CAVE_VIEW | CAVE_XTRA)) == CAVE_VIEW) {
            floor.set_note_and_redraw_at(pos);
        }

        floor.mon_lite_x[floor.mon_lite_n] = pos.x;
        floor.mon_lite_y[floor.mon_lite_n] = pos.y;
        floor.mon_lite_n++;
    }

    for (size_t i = end_temp; i < std::size(points); i++) {
        const auto &pos = points[i];
        floor.get_grid(pos).info &= ~(CAVE_TEMP | CAVE_XTRA);
    }

    RedrawingFlagsUpdater::get_instance().set_flag(StatusRecalculatingFlag::DELAY_VISIBILITY);
    player_ptr->monlite = (floor.get_grid(p_pos).info & CAVE_MNLT) != 0;
    const auto ninja_data = PlayerClass(player_ptr).get_specific_data<ninja_data_type>();
    if (!ninja_data || !ninja_data->s_stealth) {
        player_ptr->old_monlite = player_ptr->monlite;
        return;
    }

    if (player_ptr->old_monlite == player_ptr->monlite) {
        player_ptr->old_monlite = player_ptr->monlite;
        return;
    }

    if (player_ptr->monlite) {
        msg_print(_("影の覆いが薄れた気がする。", "Your mantle of shadow becomes thin."));
    } else {
        msg_print(_("影の覆いが濃くなった！", "Your mantle of shadow is restored to its original darkness."));
    }

    player_ptr->old_monlite = player_ptr->monlite;
}
