/*!
 * @brief フロア全体の処理に関するユーティリティ
 * @date 2019/04/24
 * @author deskull
 */
#include "floor/floor-util.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/floor-object.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/floor/floor-info.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "target/projection-path-calculator.h"
#include "world/world.h"
#include <range/v3/view.hpp>

/*
 * The array of floor [MAX_WID][MAX_HGT].
 * Not completely allocated, that would be inefficient
 * Not completely hardcoded, that would overflow memory
 */
FloorType floor_info;

static int scent_when = 0;

/*
 * Characters leave scent trails for perceptive monsters to track.
 *
 * Smell is rather more limited than sound.  Many creatures cannot use
 * it at all, it doesn't extend very far outwards from the character's
 * current position, and monsters can use it to home in the character,
 * but not to run away from him.
 *
 * Smell is valued according to age.  When a character takes his turn,
 * scent is aged by one, and new scent of the current age is laid down.
 * Speedy characters leave more scent, true, but it also ages faster,
 * which makes it harder to hunt them down.
 *
 * Whenever the age count loops, most of the scent trail is erased and
 * the age of the remainder is recalculated.
 */
void update_smell(FloorType &floor, const Pos2D &p_pos)
{
    /* Create a table that controls the spread of scent */
    const int scent_adjust[5][5] = {
        { -1, 0, 0, 0, -1 },
        { 0, 1, 1, 1, 0 },
        { 0, 1, 2, 1, 0 },
        { 0, 1, 1, 1, 0 },
        { -1, 0, 0, 0, -1 },
    };

    if (++scent_when == 254) {
        for (const auto &pos : floor.get_area()) {
            auto &grid = floor.get_grid(pos);
            int w = grid.when;
            grid.when = (w > 128) ? (w - 128) : 0;
        }

        scent_when = 126;
    }

    for (auto y = 0; y < 5; y++) {
        for (auto x = 0; x < 5; x++) {
            const auto pos = p_pos + Pos2DVec(y, x) + Pos2DVec(-2, -2);
            if (!floor.contains(pos)) {
                continue;
            }

            auto &grid = floor.get_grid(pos);
            auto update_when = !grid.has(TerrainCharacteristics::MOVE) && !floor.has_closed_door_at(pos);
            update_when |= !grid.has_los();
            update_when |= scent_adjust[y][x] == -1;
            if (update_when) {
                continue;
            }

            grid.when = scent_when + scent_adjust[y][x];
        }
    }
}

/*
 * Hack -- forget the "flow" information
 */
void forget_flow(FloorType &floor)
{
    for (const auto &pos : floor.get_area()) {
        auto &grid = floor.get_grid(pos);
        grid.reset_costs();
        grid.reset_dists();
        grid.when = 0;
    }
}

/*!
 * @brief グローバルオブジェクト配列を初期化する /
 * Delete all the items when player leaves the level
 * @note we do NOT visually reflect these (irrelevant) changes
 * @details
 * Hack -- we clear the "grid.o_idx" field for every grid,
 * and the "monster.next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 */
void wipe_o_list(FloorType &floor)
{
    for (const auto &[i_idx, item_ptr] : floor.o_list | ranges::views::enumerate) {
        if (!item_ptr->is_valid()) {
            continue;
        }

        if (!AngbandWorld::get_instance().character_dungeon || preserve_mode) {
            if (item_ptr->is_fixed_artifact() && !item_ptr->is_known()) {
                item_ptr->get_fixed_artifact().is_generated = false;
            }
        }

        auto &list = get_o_idx_list_contains(floor, static_cast<OBJECT_IDX>(i_idx));
        list.clear();
    }

    floor.o_list.clear();
    floor.o_list.push_back(std::make_shared<ItemEntity>()); // 0番にダミーアイテムを用意
}

/*
 * Standard "find me a location" function
 *
 * Obtains a legal location within the given distance of the initial
 * location, and with "los()" from the source to destination location.
 *
 * This function is often called from inside a loop which searches for
 * locations while increasing the "d" distance.
 *
 * Currently the "m" parameter is unused.
 */
Pos2D scatter(PlayerType *player_ptr, const Pos2D &pos, int d, uint32_t mode)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    while (true) {
        const auto ny = rand_spread(pos.y, d);
        const auto nx = rand_spread(pos.x, d);
        const Pos2D pos_neighbor(ny, nx);
        if (!floor.contains(pos_neighbor)) {
            continue;
        }
        if ((d > 1) && (Grid::calc_distance(pos, pos_neighbor) > d)) {
            continue;
        }
        if (mode & PROJECT_LOS) {
            if (los(floor, pos, pos_neighbor)) {
                return pos_neighbor;
            }

            continue;
        }

        if (projectable(floor, p_pos, pos, pos_neighbor)) {
            return pos_neighbor;
        }
    }
}

/*!
 * @brief 現在のマップ名を返す /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return マップ名の文字列参照ポインタ
 */
std::string map_name(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &quests = QuestList::get_instance();
    auto is_fixed_quest = floor.is_in_quest();
    is_fixed_quest &= QuestType::is_fixed(floor.quest_number);
    is_fixed_quest &= any_bits(quests.get_quest(floor.quest_number).flags, QUEST_FLAG_PRESET);
    if (is_fixed_quest) {
        return _("クエスト", "Quest");
    } else if (AngbandWorld::get_instance().is_wild_mode()) {
        return _("地上", "Surface");
    } else if (floor.inside_arena) {
        return _("アリーナ", "Arena");
    } else if (AngbandSystem::get_instance().is_phase_out()) {
        return _("闘技場", "Monster Arena");
    } else if (!floor.is_underground() && player_ptr->town_num) {
        return towns_info[player_ptr->town_num].name;
    } else {
        return floor.get_dungeon_definition().name;
    }
}
