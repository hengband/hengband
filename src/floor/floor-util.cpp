/*!
 * @brief フロア全体の処理に関するユーティリティ
 * @date 2019/04/24
 * @author deskull
 */
#include "floor/floor-util.h"
#include "dungeon/quest.h"
#include "effect/effect-characteristics.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-town.h"
#include "floor/geometry.h"
#include "floor/line-of-sight.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "perception/object-perception.h"
#include "system/artifact-type-definition.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/projection-path-calculator.h"
#include "util/bit-flags-calculator.h"
#include "world/world.h"

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
void update_smell(FloorType *floor_ptr, PlayerType *player_ptr)
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
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            for (POSITION x = 0; x < floor_ptr->width; x++) {
                int w = floor_ptr->grid_array[y][x].when;
                floor_ptr->grid_array[y][x].when = (w > 128) ? (w - 128) : 0;
            }
        }

        scent_when = 126;
    }

    for (POSITION i = 0; i < 5; i++) {
        for (POSITION j = 0; j < 5; j++) {
            grid_type *g_ptr;
            POSITION y = i + player_ptr->y - 2;
            POSITION x = j + player_ptr->x - 2;
            if (!in_bounds(floor_ptr, y, x)) {
                continue;
            }

            g_ptr = &floor_ptr->grid_array[y][x];
            if (!g_ptr->cave_has_flag(TerrainCharacteristics::MOVE) && !is_closed_door(player_ptr, g_ptr->feat)) {
                continue;
            }
            if (!player_has_los_bold(player_ptr, y, x)) {
                continue;
            }
            if (scent_adjust[i][j] == -1) {
                continue;
            }

            g_ptr->when = scent_when + scent_adjust[i][j];
        }
    }
}

/*
 * Hack -- forget the "flow" information
 */
void forget_flow(FloorType *floor_ptr)
{
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            memset(&floor_ptr->grid_array[y][x].costs, 0, sizeof(floor_ptr->grid_array[y][x].costs));
            memset(&floor_ptr->grid_array[y][x].dists, 0, sizeof(floor_ptr->grid_array[y][x].dists));
            floor_ptr->grid_array[y][x].when = 0;
        }
    }
}

/*!
 * @brief グローバルオブジェクト配列を初期化する /
 * Delete all the items when player leaves the level
 * @note we do NOT visually reflect these (irrelevant) changes
 * @details
 * Hack -- we clear the "g_ptr->o_idx" field for every grid,
 * and the "m_ptr->next_o_idx" field for every monster, since
 * we know we are clearing every object.  Technically, we only
 * clear those fields for grids/monsters containing objects,
 * and we clear it once for every such object.
 */
void wipe_o_list(FloorType *floor_ptr)
{
    for (OBJECT_IDX i = 1; i < floor_ptr->o_max; i++) {
        auto *o_ptr = &floor_ptr->o_list[i];
        if (!o_ptr->is_valid()) {
            continue;
        }

        if (!w_ptr->character_dungeon || preserve_mode) {
            if (o_ptr->is_fixed_artifact() && !o_ptr->is_known()) {
                ArtifactsInfo::get_instance().get_artifact(o_ptr->fixed_artifact_idx).is_generated = false;
            }
        }

        auto &list = get_o_idx_list_contains(floor_ptr, i);
        list.clear();
        o_ptr->wipe();
    }

    floor_ptr->o_max = 1;
    floor_ptr->o_cnt = 0;
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
void scatter(PlayerType *player_ptr, POSITION *yp, POSITION *xp, POSITION y, POSITION x, POSITION d, BIT_FLAGS mode)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    POSITION nx, ny;
    while (true) {
        ny = rand_spread(y, d);
        nx = rand_spread(x, d);

        if (!in_bounds(floor_ptr, ny, nx)) {
            continue;
        }
        if ((d > 1) && (distance(y, x, ny, nx) > d)) {
            continue;
        }
        if (mode & PROJECT_LOS) {
            if (los(player_ptr, y, x, ny, nx)) {
                break;
            }
            continue;
        }

        if (projectable(player_ptr, y, x, ny, nx)) {
            break;
        }
    }

    *yp = ny;
    *xp = nx;
}

/*!
 * @brief 現在のマップ名を返す /
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return マップ名の文字列参照ポインタ
 */
concptr map_name(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    const auto &quest_list = QuestList::get_instance();
    auto is_fixed_quest = inside_quest(floor_ptr->quest_number);
    is_fixed_quest &= quest_type::is_fixed(floor_ptr->quest_number);
    is_fixed_quest &= any_bits(quest_list[floor_ptr->quest_number].flags, QUEST_FLAG_PRESET);
    if (is_fixed_quest) {
        return _("クエスト", "Quest");
    } else if (player_ptr->wild_mode) {
        return _("地上", "Surface");
    } else if (floor_ptr->inside_arena) {
        return _("アリーナ", "Arena");
    } else if (player_ptr->phase_out) {
        return _("闘技場", "Monster Arena");
    } else if (!floor_ptr->dun_level && player_ptr->town_num) {
        return towns_info[player_ptr->town_num].name;
    } else {
        return dungeons_info[player_ptr->dungeon_idx].name.data();
    }
}
