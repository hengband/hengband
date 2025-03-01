#include "spell-kind/spells-lite.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "floor/cave.h"
#include "floor/floor-util.h"
#include "game-option/map-screen-options.h"
#include "grid/grid.h"
#include "mind/mind-ninja.h"
#include "monster/monster-describer.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-update.h"
#include "spell-kind/spells-launcher.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "timed-effect/timed-effects.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <range/v3/view.hpp>
#include <vector>

/*!
 * @brief 指定した座標全てを照らす。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param positions 明るくすべき座標群
 * @details
 * <pre>
 * This routine clears the entire "temp" set.
 * This routine will Perma-Lite all "temp" grids.
 * This routine is used (only) by "lite_room()"
 * Dark grids are illuminated.
 * Also, process all affected monsters.
 *
 * SMART monsters always wake up when illuminated
 * NORMAL monsters wake up 1/4 the time when illuminated
 * STUPID monsters wake up 1/10 the time when illuminated
 * </pre>
 */
static void cave_temp_room_lite(PlayerType *player_ptr, const std::vector<Pos2D> &positions)
{
    auto &floor = *player_ptr->current_floor_ptr;
    for (const auto &pos : positions) {
        auto &grid = floor.get_grid(pos);
        grid.info &= ~(CAVE_TEMP);
        grid.info |= (CAVE_GLOW);
        if (grid.has_monster()) {
            auto chance = 25;
            const auto &monster = floor.m_list[grid.m_idx];
            const auto &monrace = monster.get_monrace();
            update_monster(player_ptr, grid.m_idx, false);
            if (monrace.behavior_flags.has(MonsterBehaviorType::STUPID)) {
                chance = 10;
            }
            if (monrace.behavior_flags.has(MonsterBehaviorType::SMART)) {
                chance = 100;
            }

            if (monster.is_asleep() && evaluate_percent(chance)) {
                (void)set_monster_csleep(player_ptr, grid.m_idx, 0);
                if (monster.ml) {
                    const auto m_name = monster_desc(player_ptr, monster, 0);
                    msg_format(_("%s^が目を覚ました。", "%s^ wakes up."), m_name.data());
                }
            }
        }

        note_spot(player_ptr, pos);
        lite_spot(player_ptr, pos);
        update_local_illumination(player_ptr, pos);
    }
}

/*!
 * @brief 指定した座標全てを暗くする。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param points 暗くすべき座標群
 */
static void cave_temp_room_unlite(PlayerType *player_ptr, const std::vector<Pos2D> &positions)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &world = AngbandWorld::get_instance();
    for (const auto &pos : positions) {
        auto &grid = floor.get_grid(pos);
        auto do_dark = !grid.is_mirror();
        grid.info &= ~(CAVE_TEMP);
        if (!do_dark) {
            continue;
        }

        if (floor.is_underground() || !world.is_daytime()) {
            for (const auto &d : Direction::directions()) {
                const Pos2D pos_neighbor = pos + d.vec();
                if (!floor.contains(pos_neighbor, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
                    continue;
                }

                const auto &grid_neighbor = floor.get_grid(pos_neighbor);
                if (grid_neighbor.get_terrain(TerrainKind::MIMIC).flags.has(TerrainCharacteristics::GLOW)) {
                    do_dark = false;
                    break;
                }
            }

            if (!do_dark) {
                continue;
            }
        }

        grid.info &= ~(CAVE_GLOW);
        if (grid.get_terrain(TerrainKind::MIMIC).flags.has_not(TerrainCharacteristics::REMEMBER)) {
            if (!view_torch_grids) {
                grid.info &= ~(CAVE_MARK);
            }
            note_spot(player_ptr, pos);
        }

        if (grid.has_monster()) {
            update_monster(player_ptr, grid.m_idx, false);
        }

        lite_spot(player_ptr, pos);
        update_local_illumination(player_ptr, pos);
    }
}

/*!
 * @brief 周辺に関数ポインタの条件に該当する地形がいくつあるかを計算する
 * @param floor フロアへの参照
 * @param pos_center 中心座標
 * @param tc 地形条件
 * @return 該当地形の数
 */
static int next_to_open(const FloorType &floor, const Pos2D &pos_center, TerrainCharacteristics tc)
{
    auto len = 0;
    auto blen = 0;
    for (const auto cdir : ranges::views::iota(0, 8) | ranges::views::cycle | ranges::views::take(16)) {
        const auto pos = pos_center + Direction::from_cdir(cdir).vec();
        if (floor.check_path(pos, tc)) {
            len++;
            continue;
        }

        if (len > blen) {
            blen = len;
        }

        len = 0;
    }

    return std::max(len, blen);
}

/*!
 * @brief 周辺に関数ポインタの条件に該当する地形がいくつあるかを計算する
 * @param floor フロアへの参照
 * @param pos_center 中心座標
 * @param tc 地形条件
 * @return 該当地形の数
 */
static int next_to_walls_adj(const FloorType &floor, const Pos2D &pos_center, TerrainCharacteristics tc)
{
    auto count = 0;
    for (const auto &d : Direction::directions_8()) {
        const auto pos = pos_center + d.vec();
        if (!floor.check_path(pos, tc)) {
            count++;
        }
    }

    return count;
}

/*!
 * @brief pos が指定条件を満たすかをチェックする
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 部屋内の座標
 * @param tc 明るくするならLOS、暗くするならPROJECTION
 * @return 明るくするか暗くするならtrue、何もしないならfalse
 * @details The reason why it is ==6 instead of >5 is that 8 is impossible due to the check for cave_bold above.
 * 7 lights dead-end corridors (you need to do this for the checkboard interesting rooms, so that the boundary is lit properly.
 * This leaves only a check for 6 bounding walls!
 */
static bool cave_temp_room_aux(const FloorType &floor, const Pos2D &pos, const Pos2D &p_pos, TerrainCharacteristics tc)
{
    auto &grid = floor.get_grid(pos);
    if (any_bits(grid.info, CAVE_TEMP)) {
        return false;
    }

    if (any_bits(grid.info, CAVE_ROOM)) {
        return true;
    }

    if (tc == TerrainCharacteristics::PROJECTION) {
        return false;
    }

    if (!floor.contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        return false;
    }

    if (Grid::calc_distance(p_pos, pos) > AngbandSystem::get_instance().get_max_range()) {
        return false;
    }

    if (floor.contains(pos) && floor.check_path(pos, tc) && (next_to_walls_adj(floor, pos, tc) == 6) && (next_to_open(floor, pos, tc) <= 1)) {
        return false;
    }

    return true;
}

/*!
 * @brief 起点座標が含まれる部屋を照らす (部屋でない場合は起点周辺)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos_start 起点座標
 * @details pos_start を起点として明るくするマスを記録していく. 実質幅優先探索.
 */
void lite_room(PlayerType *player_ptr, const Pos2D &pos_start)
{
    std::vector<Pos2D> positions;
    auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    if (cave_temp_room_aux(floor, pos_start, p_pos, TerrainCharacteristics::LOS)) {
        floor.get_grid(pos_start).info |= CAVE_TEMP;
        positions.push_back(pos_start);
    }

    for (size_t i = 0; i < positions.size(); i++) {
        const Pos2D pos = positions[i];
        if (!floor.has_los_terrain_at(pos)) {
            continue;
        }

        for (const auto &d : Direction::directions_8()) {
            const auto pos_neighbor = pos + d.vec();
            if (cave_temp_room_aux(floor, pos_neighbor, p_pos, TerrainCharacteristics::LOS)) {
                floor.get_grid(pos_neighbor).info |= CAVE_TEMP;
                positions.push_back(pos_neighbor);
            }
        }
    }

    cave_temp_room_lite(player_ptr, positions);
    if (floor.grid_array[player_ptr->y][player_ptr->x].info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }
}

/*!
 * @brief 起点座標が含まれる部屋を暗くする (部屋でない場合は起点周辺)
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos_start 指定座標
 * @details pos_start を起点として暗くするマスを記録していく. 実質幅優先探索.
 */
void unlite_room(PlayerType *player_ptr, const Pos2D &pos_start)
{
    std::vector<Pos2D> positions;
    auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    if (cave_temp_room_aux(floor, pos_start, p_pos, TerrainCharacteristics::PROJECTION)) {
        floor.get_grid(pos_start).info |= CAVE_TEMP;
        positions.push_back(pos_start);
    }

    for (size_t i = 0; i < positions.size(); i++) {
        const Pos2D pos = positions[i];
        if (!floor.has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION)) {
            continue;
        }

        for (const auto &d : Direction::directions_8()) {
            const auto pos_neighbor = pos + d.vec();
            if (cave_temp_room_aux(floor, pos_neighbor, p_pos, TerrainCharacteristics::PROJECTION)) {
                floor.get_grid(pos_neighbor).info |= CAVE_TEMP;
                positions.push_back(pos_neighbor);
            }
        }
    }

    cave_temp_room_unlite(player_ptr, positions);
}

/*!
 * @brief スターライトの効果を発生させる
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param magic 魔法による効果であればTRUE、スターライトの杖による効果であればFALSE
 * @return 常にTRUE
 */
bool starlight(PlayerType *player_ptr, bool magic)
{
    if (!player_ptr->effects()->blindness().is_blind() && !magic) {
        msg_print(_("杖の先が明るく輝いた...", "The end of the staff glows brightly..."));
    }

    const auto p_pos = player_ptr->get_position();
    const auto num = Dice::roll(5, 3);
    for (auto k = 0; k < num; k++) {
        Pos2D pos(0, 0);
        auto attempts = 1000;
        while (attempts--) {
            pos = scatter(player_ptr, p_pos, 4, PROJECT_LOS);
            if (!player_ptr->current_floor_ptr->has_terrain_characteristics(pos, TerrainCharacteristics::PROJECTION)) {
                continue;
            }

            if (!player_ptr->is_located_at(pos)) {
                break;
            }
        }

        constexpr uint flags = PROJECT_BEAM | PROJECT_THRU | PROJECT_GRID | PROJECT_KILL | PROJECT_LOS;
        project(player_ptr, 0, 0, pos.y, pos.x, Dice::roll(6 + player_ptr->lev / 8, 10), AttributeType::LITE_WEAK, flags);
    }

    return true;
}

/*!
 * @brief プレイヤー位置を中心にLITE_WEAK属性を通じた照明処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_area(PlayerType *player_ptr, int dam, int rad)
{
    if (player_ptr->current_floor_ptr->get_dungeon_definition().flags.has(DungeonFeatureType::DARKNESS)) {
        msg_print(_("ダンジョンが光を吸収した。", "The darkness of this dungeon absorbs your light."));
        return false;
    }

    if (!player_ptr->effects()->blindness().is_blind()) {
        msg_print(_("白い光が辺りを覆った。", "You are surrounded by a white light."));
    }

    BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;
    (void)project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, dam, AttributeType::LITE_WEAK, flg);

    lite_room(player_ptr, player_ptr->get_position());

    return true;
}

/*!
 * @brief プレイヤー位置を中心にLITE_DARK属性を通じた消灯処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dam 威力
 * @param rad 効果半径
 * @return 作用が実際にあった場合TRUEを返す
 */
bool unlite_area(PlayerType *player_ptr, int dam, int rad)
{
    if (!player_ptr->effects()->blindness().is_blind()) {
        msg_print(_("暗闇が辺りを覆った。", "Darkness surrounds you."));
    }

    BIT_FLAGS flg = PROJECT_GRID | PROJECT_KILL;
    (void)project(player_ptr, 0, rad, player_ptr->y, player_ptr->x, dam, AttributeType::DARK_WEAK, flg);

    unlite_room(player_ptr, player_ptr->get_position());

    return true;
}

/*!
 * @brief LITE_WEAK属性による光源ビーム処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param dir 方向(5ならばグローバル変数 target_col/target_row の座標を目標にする)
 * @param dam 威力
 * @return 作用が実際にあった場合TRUEを返す
 */
bool lite_line(PlayerType *player_ptr, const Direction &dir, int dam)
{
    BIT_FLAGS flg = PROJECT_BEAM | PROJECT_GRID | PROJECT_KILL;
    return project_hook(player_ptr, AttributeType::LITE_WEAK, dir, dam, flg);
}
