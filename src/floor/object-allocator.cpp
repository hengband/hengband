#include "floor/object-allocator.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-generator-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/object-placer.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "wizard/wizard-messages.h"
#include <range/v3/view.hpp>

/*!
 * @brief 上下左右の外壁数をカウントする
 * @param pos 基準座標
 * @return 隣接する外壁の数
 */
static int next_to_walls(const FloorType &floor, const Pos2D &pos)
{
    auto k = 0;
    for (const auto &d : Direction::directions_4()) {
        const auto pos_neighbor = pos + d.vec();
        if (floor.contains(pos_neighbor) && floor.get_grid(pos_neighbor).is_extra()) {
            k++;
        }
    }

    return k;
}

/*!
 * @brief alloc_stairs()の補助として指定の位置に階段を生成できるかの判定を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 基準座標
 * @param walls 最低減隣接させたい外壁の数
 * @return 階段を生成して問題がないならばTRUEを返す。
 */
static bool alloc_stairs_aux(PlayerType *player_ptr, const Pos2D &pos, int walls)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    if (!grid.is_floor() || grid.has(TerrainCharacteristics::PATTERN) || !grid.o_idx_list.empty() || grid.has_monster() || next_to_walls(floor, pos) < walls) {
        return false;
    }

    return true;
}

/*!
 * @brief 外壁に隣接させて階段を生成する / Places some staircases near walls
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param feat 配置したい地形ID
 * @param num 配置したい階段の数
 * @param walls 最低減隣接させたい外壁の数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
bool alloc_stairs(PlayerType *player_ptr, FEAT_IDX feat, int num, int walls)
{
    int shaft_num = 0;
    const auto &terrain = TerrainList::get_instance().get_terrain(feat);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    if (terrain.flags.has(TerrainCharacteristics::LESS)) {
        if (ironman_downward || !floor.is_underground()) {
            return true;
        }

        if (floor.dun_level > dungeon.mindepth) {
            shaft_num = (randint1(num + 1)) / 2;
        }
    } else if (terrain.flags.has(TerrainCharacteristics::MORE)) {
        auto quest_id = floor.get_quest_id();
        const auto &quests = QuestList::get_instance();
        if (floor.dun_level > 1 && inside_quest(quest_id)) {
            const auto &monrace = quests.get_quest(quest_id).get_bounty();
            if (!monrace.is_dead_unique()) {
                return true;
            }
        }

        if (floor.dun_level >= dungeon.maxdepth) {
            return true;
        }

        if ((floor.dun_level < dungeon.maxdepth - 1) && !inside_quest(floor.get_quest_id(1))) {
            shaft_num = (randint1(num) + 1) / 2;
        }
    } else {
        return false;
    }

    for (auto i = 0; i < num; i++) {
        while (true) {
            const auto can_alloc_stair = [&](const Pos2D &pos) { return alloc_stairs_aux(player_ptr, pos, walls); };
            const auto pos_candidates =
                floor.get_area(FloorBoundary::OUTER_WALL_INCLUSIVE) |
                ranges::views::filter(can_alloc_stair) |
                ranges::to_vector;

            if (pos_candidates.empty()) {
                if (walls <= 0) {
                    return false;
                }

                walls--;
                continue;
            }

            const auto &pos_stair = rand_choice(pos_candidates);
            auto &grid = floor.get_grid(pos_stair);
            grid.mimic = 0;
            grid.feat = (i < shaft_num) ? dungeon.convert_terrain_id(feat, TerrainCharacteristics::SHAFT) : feat;
            grid.info &= ~(CAVE_FLOOR);
            break;
        }
    }

    return true;
}

/*!
 * @brief フロア上のランダム位置に各種オブジェクトを配置する / Allocates some objects (using "place" and "type")
 * @param set 配置したい地形の種類
 * @param typ 配置したいオブジェクトの種類
 * @param num 配置したい数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
void alloc_object(PlayerType *player_ptr, dap_type set, dungeon_allocation_type typ, int num)
{
    auto dummy = 0;
    auto &floor = *player_ptr->current_floor_ptr;
    num = num * floor.height * floor.width / (MAX_HGT * MAX_WID) + 1;
    for (auto k = 0; k < num; k++) {
        Pos2D pos(0, 0);
        while (dummy < SAFE_MAX_ATTEMPTS) {
            dummy++;
            pos.y = randint0(floor.height);
            pos.x = randint0(floor.width);
            const auto &grid = floor.get_grid(pos);
            if (!grid.is_floor() || !grid.o_idx_list.empty() || grid.has_monster()) {
                continue;
            }

            if (player_ptr->is_located_at(pos)) {
                continue;
            }

            auto is_room = grid.is_room();
            if (((set == ALLOC_SET_CORR) && is_room) || ((set == ALLOC_SET_ROOM) && !is_room)) {
                continue;
            }

            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS) {
            msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("アイテムの配置に失敗しました。", "Failed to place object."));
            return;
        }

        switch (typ) {
        case ALLOC_TYP_RUBBLE:
            floor.set_terrain_id_at(pos, TerrainTag::RUBBLE);
            floor.get_grid(pos).info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_TRAP:
            floor.place_trap_at(pos);
            floor.get_grid(pos).info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_GOLD:
            place_gold(player_ptr, pos);
            break;
        case ALLOC_TYP_OBJECT:
            place_object(player_ptr, pos, 0);
            break;
        default:
            break;
        }
    }
}
