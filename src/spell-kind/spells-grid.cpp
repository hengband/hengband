#include "spell-kind/spells-grid.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "grid/stair.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 守りのルーン設置処理 /
 * Leave a "rune of protection" which prevents monster movement
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool create_rune_protection_one(PlayerType *player_ptr)
{
    if (!cave_clean_bold(player_ptr->current_floor_ptr, player_ptr->y, player_ptr->x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return false;
    }

    player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].info |= CAVE_OBJECT;
    player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x].mimic = feat_rune_protection;
    note_spot(player_ptr, player_ptr->y, player_ptr->x);
    lite_spot(player_ptr, player_ptr->y, player_ptr->x);
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
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!cave_clean_bold(floor_ptr, y, x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return false;
    }

    floor_ptr->grid_array[y][x].info |= CAVE_OBJECT;
    floor_ptr->grid_array[y][x].mimic = feat_rune_explosion;
    note_spot(player_ptr, y, x);
    lite_spot(player_ptr, y, x);
    return true;
}

/*!
 * @brief プレイヤーの手による能動的な階段生成処理 /
 * Create stairs at or move previously created stairs into the player location.
 */
void stair_creation(PlayerType *player_ptr)
{
    bool up = true;
    if (ironman_downward)
        up = false;

    bool down = true;
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (inside_quest(quest_number(player_ptr, floor_ptr->dun_level)) || (floor_ptr->dun_level >= d_info[player_ptr->dungeon_idx].maxdepth))
        down = false;

    if (!floor_ptr->dun_level || (!up && !down) || (inside_quest(floor_ptr->quest_number) && quest_type::is_fixed(floor_ptr->quest_number)) || floor_ptr->inside_arena || player_ptr->phase_out) {
        msg_print(_("効果がありません！", "There is no effect!"));
        return;
    }

    if (!cave_valid_bold(floor_ptr, player_ptr->y, player_ptr->x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return;
    }

    delete_all_items_from_floor(player_ptr, player_ptr->y, player_ptr->x);
    saved_floor_type *sf_ptr;
    sf_ptr = get_sf_ptr(player_ptr->floor_id);
    if (!sf_ptr) {
        player_ptr->floor_id = get_new_floor_id(player_ptr);
        sf_ptr = get_sf_ptr(player_ptr->floor_id);
    }

    if (up && down) {
        if (randint0(100) < 50)
            up = false;
        else
            down = false;
    }

    FLOOR_IDX dest_floor_id = 0;
    if (up) {
        if (sf_ptr->upper_floor_id)
            dest_floor_id = sf_ptr->upper_floor_id;
    } else {
        if (sf_ptr->lower_floor_id)
            dest_floor_id = sf_ptr->lower_floor_id;
    }

    if (dest_floor_id) {
        for (POSITION y = 0; y < floor_ptr->height; y++) {
            for (POSITION x = 0; x < floor_ptr->width; x++) {
                auto *g_ptr = &floor_ptr->grid_array[y][x];
                if (!g_ptr->special)
                    continue;
                if (feat_uses_special(g_ptr->feat))
                    continue;
                if (g_ptr->special != dest_floor_id)
                    continue;

                /* Remove old stairs */
                g_ptr->special = 0;
                cave_set_feat(player_ptr, y, x, feat_ground_type[randint0(100)]);
            }
        }
    } else {
        dest_floor_id = get_new_floor_id(player_ptr);
        if (up)
            sf_ptr->upper_floor_id = dest_floor_id;
        else
            sf_ptr->lower_floor_id = dest_floor_id;
    }

    saved_floor_type *dest_sf_ptr;
    dest_sf_ptr = get_sf_ptr(dest_floor_id);
    if (up) {
        cave_set_feat(player_ptr, player_ptr->y, player_ptr->x,
            (dest_sf_ptr->last_visit && (dest_sf_ptr->dun_level <= floor_ptr->dun_level - 2)) ? feat_state(player_ptr->current_floor_ptr, feat_up_stair, FloorFeatureType::SHAFT)
                                                                                              : feat_up_stair);
    } else {
        cave_set_feat(player_ptr, player_ptr->y, player_ptr->x,
            (dest_sf_ptr->last_visit && (dest_sf_ptr->dun_level >= floor_ptr->dun_level + 2)) ? feat_state(player_ptr->current_floor_ptr, feat_down_stair, FloorFeatureType::SHAFT)
                                                                                              : feat_down_stair);
    }

    floor_ptr->grid_array[player_ptr->y][player_ptr->x].special = dest_floor_id;
}
