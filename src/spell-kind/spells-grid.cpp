﻿#include "spell-kind/spells-grid.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-object.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "game-option/birth-options.h"
#include "grid/grid.h"
#include "grid/stair.h"
#include "system/floor-type-definition.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief 守りのルーン設置処理 /
 * Leave a "rune of protection" which prevents monster movement
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool create_rune_protection_one(player_type *caster_ptr)
{
    if (!cave_clean_bold(caster_ptr->current_floor_ptr, caster_ptr->y, caster_ptr->x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return FALSE;
    }

    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].info |= CAVE_OBJECT;
    caster_ptr->current_floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].mimic = feat_rune_protection;
    note_spot(caster_ptr, caster_ptr->y, caster_ptr->x);
    lite_spot(caster_ptr, caster_ptr->y, caster_ptr->x);
    return TRUE;
}

/*!
 * @brief 爆発のルーン設置処理 /
 * Leave an "explosive rune" which prevents monster movement
 * @param caster_ptr プレーヤーへの参照ポインタ
 * @param y 設置場所
 * @param x 設置場所
 * @return 実際に設置が行われた場合TRUEを返す
 */
bool create_rune_explosion(player_type *caster_ptr, POSITION y, POSITION x)
{
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    if (!cave_clean_bold(floor_ptr, y, x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return FALSE;
    }

    floor_ptr->grid_array[y][x].info |= CAVE_OBJECT;
    floor_ptr->grid_array[y][x].mimic = feat_rune_explosion;
    note_spot(caster_ptr, y, x);
    lite_spot(caster_ptr, y, x);
    return TRUE;
}

/*!
 * @brief プレイヤーの手による能動的な階段生成処理 /
 * Create stairs at or move previously created stairs into the player location.
 * @return なし
 */
void stair_creation(player_type *caster_ptr)
{
    bool up = TRUE;
    if (ironman_downward)
        up = FALSE;

    bool down = TRUE;
    floor_type *floor_ptr = caster_ptr->current_floor_ptr;
    if (quest_number(caster_ptr, floor_ptr->dun_level) || (floor_ptr->dun_level >= d_info[caster_ptr->dungeon_idx].maxdepth))
        down = FALSE;

    if (!floor_ptr->dun_level || (!up && !down) || (floor_ptr->inside_quest && is_fixed_quest_idx(floor_ptr->inside_quest)) || floor_ptr->inside_arena
        || caster_ptr->phase_out) {
        msg_print(_("効果がありません！", "There is no effect!"));
        return;
    }

    if (!cave_valid_bold(floor_ptr, caster_ptr->y, caster_ptr->x)) {
        msg_print(_("床上のアイテムが呪文を跳ね返した。", "The object resists the spell."));
        return;
    }

    delete_all_items_from_floor(caster_ptr, caster_ptr->y, caster_ptr->x);
    saved_floor_type *sf_ptr;
    sf_ptr = get_sf_ptr(caster_ptr->floor_id);
    if (!sf_ptr) {
        caster_ptr->floor_id = get_new_floor_id(caster_ptr);
        sf_ptr = get_sf_ptr(caster_ptr->floor_id);
    }

    if (up && down) {
        if (randint0(100) < 50)
            up = FALSE;
        else
            down = FALSE;
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
                grid_type *g_ptr = &floor_ptr->grid_array[y][x];
                if (!g_ptr->special)
                    continue;
                if (feat_uses_special(g_ptr->feat))
                    continue;
                if (g_ptr->special != dest_floor_id)
                    continue;

                /* Remove old stairs */
                g_ptr->special = 0;
                cave_set_feat(caster_ptr, y, x, feat_ground_type[randint0(100)]);
            }
        }
    } else {
        dest_floor_id = get_new_floor_id(caster_ptr);
        if (up)
            sf_ptr->upper_floor_id = dest_floor_id;
        else
            sf_ptr->lower_floor_id = dest_floor_id;
    }

    saved_floor_type *dest_sf_ptr;
    dest_sf_ptr = get_sf_ptr(dest_floor_id);
    if (up) {
        cave_set_feat(caster_ptr, caster_ptr->y, caster_ptr->x,
            (dest_sf_ptr->last_visit && (dest_sf_ptr->dun_level <= floor_ptr->dun_level - 2)) ? feat_state(caster_ptr, feat_up_stair, FF_SHAFT)
                                                                                              : feat_up_stair);
    } else {
        cave_set_feat(caster_ptr, caster_ptr->y, caster_ptr->x,
            (dest_sf_ptr->last_visit && (dest_sf_ptr->dun_level >= floor_ptr->dun_level + 2)) ? feat_state(caster_ptr, feat_down_stair, FF_SHAFT)
                                                                                              : feat_down_stair);
    }

    floor_ptr->grid_array[caster_ptr->y][caster_ptr->x].special = dest_floor_id;
}
