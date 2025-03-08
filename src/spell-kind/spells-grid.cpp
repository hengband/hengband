#include "spell-kind/spells-grid.h"
#include "dungeon/quest.h"
#include "floor/floor-object.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "view/display-messages.h"

/*!
 * @brief 守りのルーン設置処理 /
 * Leave a "rune of protection" which prevents monster movement
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool create_rune_protection_one(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;
    const auto p_pos = player_ptr->get_position();
    if (!floor.is_clean_at(p_pos)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return false;
    }

    auto &grid = floor.get_grid(p_pos);
    grid.info |= CAVE_OBJECT;
    grid.set_terrain_id(TerrainTag::RUNE_PROTECTION, TerrainKind::MIMIC);
    note_spot(player_ptr, p_pos);
    lite_spot(player_ptr, p_pos);
    return true;
}

/*!
 * @brief 爆発のルーン設置処理 /
 * Leave an "explosive rune" which prevents monster movement
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 設置場所
 * @param x 設置場所
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool create_rune_explosion(PlayerType *player_ptr, POSITION y, POSITION x)
{
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    if (!floor.is_clean_at(pos)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return false;
    }

    auto &grid = floor.get_grid(pos);
    grid.info |= CAVE_OBJECT;
    grid.set_terrain_id(TerrainTag::RUNE_EXPLOSION, TerrainKind::MIMIC);
    note_spot(player_ptr, pos);
    lite_spot(player_ptr, pos);
    return true;
}

/*!
 * @brief プレイヤーの手による能動的な階段生成処理 /
 * Create stairs at or move previously created stairs into the player location.
 */
void stair_creation(PlayerType *player_ptr)
{
    auto up = !ironman_downward;
    auto &floor = *player_ptr->current_floor_ptr;
    auto down = !inside_quest(floor.get_quest_id()) && (floor.dun_level < floor.get_dungeon_definition().maxdepth);
    if (!floor.is_underground() || (!up && !down) || (floor.is_in_quest() && QuestType::is_fixed(floor.quest_number)) || floor.inside_arena || AngbandSystem::get_instance().is_phase_out()) {
        msg_print(_("効果がありません！", "There is no effect!"));
        return;
    }

    const auto p_pos = player_ptr->get_position();
    if (!floor.is_grid_changeable(p_pos)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return;
    }

    delete_all_items_from_floor(player_ptr, player_ptr->get_position());
    auto *sf_ptr = get_sf_ptr(player_ptr->floor_id);
    if (!sf_ptr) {
        player_ptr->floor_id = get_unused_floor_id(player_ptr);
        sf_ptr = get_sf_ptr(player_ptr->floor_id);
    }

    if (up && down) {
        if (one_in_(2)) {
            up = false;
        } else {
            down = false;
        }
    }

    short dest_floor_id = 0;
    if (up) {
        if (sf_ptr->upper_floor_id) {
            dest_floor_id = sf_ptr->upper_floor_id;
        }
    } else {
        if (sf_ptr->lower_floor_id) {
            dest_floor_id = sf_ptr->lower_floor_id;
        }
    }

    const auto &dungeon = floor.get_dungeon_definition();
    if (dest_floor_id) {
        for (const auto &pos : floor.get_area()) {
            auto &grid = floor.get_grid(pos);
            if (!grid.special) {
                continue;
            }

            if (grid.has_special_terrain()) {
                continue;
            }

            if (grid.special != dest_floor_id) {
                continue;
            }

            /* Remove old stairs */
            grid.special = 0;
            set_terrain_id_to_grid(player_ptr, pos, dungeon.select_floor_terrain_id());
        }
    } else {
        dest_floor_id = get_unused_floor_id(player_ptr);
        if (up) {
            sf_ptr->upper_floor_id = dest_floor_id;
        } else {
            sf_ptr->lower_floor_id = dest_floor_id;
        }
    }

    const auto *dest_sf_ptr = get_sf_ptr(dest_floor_id);
    const auto &terrains = TerrainList::get_instance();
    if (up) {
        const auto is_shallow = dest_sf_ptr->dun_level <= floor.dun_level - 2;
        const auto terrain_up_stair = terrains.get_terrain_id(TerrainTag::UP_STAIR);
        const auto should_convert = (dest_sf_ptr->last_visit > 0) && is_shallow;
        const auto converted_terrain_id = dungeon.convert_terrain_id(terrain_up_stair, TerrainCharacteristics::SHAFT);
        const auto terrain_id = should_convert ? converted_terrain_id : terrain_up_stair;
        set_terrain_id_to_grid(player_ptr, player_ptr->get_position(), terrain_id);
    } else {
        const auto is_deep = dest_sf_ptr->dun_level >= floor.dun_level + 2;
        const auto terrain_down_stair = terrains.get_terrain_id(TerrainTag::DOWN_STAIR);
        const auto should_convert = (dest_sf_ptr->last_visit > 0) && is_deep;
        const auto converted_terrain_id = dungeon.convert_terrain_id(terrain_down_stair, TerrainCharacteristics::SHAFT);
        const auto terrain_id = should_convert ? converted_terrain_id : terrain_down_stair;
        set_terrain_id_to_grid(player_ptr, player_ptr->get_position(), terrain_id);
    }

    floor.get_grid(player_ptr->get_position()).special = dest_floor_id;
}
