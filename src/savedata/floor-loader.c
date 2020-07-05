#include "savedata/floor-loader.h"
#include "floor/floor-generate.h"
#include "floor/floor.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "object-hook/hook-checker.h"
#include "savedata/angband-version-comparer.h"
#include "savedata/item-loader.h"
#include "savedata/monster-loader.h"
#include "savedata/load-util.h"
#include "savedata/load-v1-5-0.h"
#include "savedata/old-feature-types.h"
#include "system/object-type-definition.h"
#include "world/world-object.h"
#include "world/world.h"

/*!
 * @brief 保存されたフロアを読み込む / Read the saved floor
 * @param player_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 最後に保存されたフロアへの参照ポインタ
 * @return info読み込みエラーコード
 * @details
 * この関数は、セーブデータの互換性を保つために多くのデータ改変処理を備えている。
 * 現在確認している処理は以下の通り、
 * <ul>
 * <li>1.7.0.2で8bitだったgrid_typeのfeat,mimicのID値を16bitに拡張する処理。</li>
 * <li>1.7.0.8までに廃止、IDなどを差し替えたクエスト番号を置換する処理。</li>
 * </ul>
 * The monsters/objects must be loaded in the same order
 * that they were stored, since the actual indexes matter.
 */
errr rd_saved_floor(player_type *player_ptr, saved_floor_type *sf_ptr)
{
    grid_template_type *templates;
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    clear_cave(player_ptr);
    player_ptr->x = player_ptr->y = 0;

    if (!sf_ptr) {
        s16b tmp16s;
        rd_s16b(&tmp16s);
        floor_ptr->dun_level = (DEPTH)tmp16s;
        floor_ptr->base_level = floor_ptr->dun_level;
    } else {
        s16b tmp16s;
        rd_s16b(&tmp16s);
        if (tmp16s != sf_ptr->floor_id)
            return 171;

        byte tmp8u;
        rd_byte(&tmp8u);
        if (tmp8u != sf_ptr->savefile_id)
            return 171;

        rd_s16b(&tmp16s);
        if (tmp16s != sf_ptr->dun_level)
            return 171;
        floor_ptr->dun_level = sf_ptr->dun_level;

        s32b tmp32s;
        rd_s32b(&tmp32s);
        if (tmp32s != sf_ptr->last_visit)
            return 171;

        u32b tmp32u;
        rd_u32b(&tmp32u);
        if (tmp32u != sf_ptr->visit_mark)
            return 171;

        rd_s16b(&tmp16s);
        if (tmp16s != sf_ptr->upper_floor_id)
            return 171;

        rd_s16b(&tmp16s);
        if (tmp16s != sf_ptr->lower_floor_id)
            return 171;
    }

    s16b tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->base_level = (DEPTH)tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->num_repro = (MONSTER_NUMBER)tmp16s;

    u16b tmp16u;
    rd_u16b(&tmp16u);
    player_ptr->y = (POSITION)tmp16u;

    rd_u16b(&tmp16u);
    player_ptr->x = (POSITION)tmp16u;

    rd_s16b(&tmp16s);
    floor_ptr->height = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->width = (POSITION)tmp16s;

    rd_byte(&player_ptr->feeling);

    u16b limit;
    rd_u16b(&limit);
    C_MAKE(templates, limit, grid_template_type);

    for (int i = 0; i < limit; i++) {
        grid_template_type *ct_ptr = &templates[i];
        rd_u16b(&tmp16u);
        ct_ptr->info = (BIT_FLAGS)tmp16u;
        if (h_older_than(1, 7, 0, 2)) {
            byte tmp8u;
            rd_byte(&tmp8u);
            ct_ptr->feat = (s16b)tmp8u;
            rd_byte(&tmp8u);
            ct_ptr->mimic = (s16b)tmp8u;
        } else {
            rd_s16b(&ct_ptr->feat);
            rd_s16b(&ct_ptr->mimic);
        }

        rd_s16b(&ct_ptr->special);
    }

    POSITION ymax = floor_ptr->height;
    POSITION xmax = floor_ptr->width;
    for (POSITION x = 0, y = 0; y < ymax;) {
        byte count;
        rd_byte(&count);

        u16b id = 0;
        byte tmp8u;
        do {
            rd_byte(&tmp8u);
            id += tmp8u;
        } while (tmp8u == MAX_UCHAR);

        for (int i = count; i > 0; i--) {
            grid_type *g_ptr = &floor_ptr->grid_array[y][x];
            g_ptr->info = templates[id].info;
            g_ptr->feat = templates[id].feat;
            g_ptr->mimic = templates[id].mimic;
            g_ptr->special = templates[id].special;

            if (++x >= xmax) {
                x = 0;
                if (++y >= ymax)
                    break;
            }
        }
    }

    /* Quest 18 was removed */
    if (h_older_than(1, 7, 0, 6) && !vanilla_town) {
        for (POSITION y = 0; y < ymax; y++) {
            for (POSITION x = 0; x < xmax; x++) {
                grid_type *g_ptr = &floor_ptr->grid_array[y][x];

                if ((g_ptr->special == OLD_QUEST_WATER_CAVE) && !floor_ptr->dun_level) {
                    if (g_ptr->feat == OLD_FEAT_QUEST_ENTER) {
                        g_ptr->feat = feat_tree;
                        g_ptr->special = 0;
                    } else if (g_ptr->feat == OLD_FEAT_BLDG_1) {
                        g_ptr->special = lite_town ? QUEST_OLD_CASTLE : QUEST_ROYAL_CRYPT;
                    }
                } else if ((g_ptr->feat == OLD_FEAT_QUEST_EXIT) && (floor_ptr->inside_quest == OLD_QUEST_WATER_CAVE)) {
                    g_ptr->feat = feat_up_stair;
                    g_ptr->special = 0;
                }
            }
        }
    }

    C_KILL(templates, limit, grid_template_type);
    rd_u16b(&limit);
    if (limit > current_world_ptr->max_o_idx)
        return 151;
    for (int i = 1; i < limit; i++) {
        OBJECT_IDX o_idx;
        object_type *o_ptr;
        o_idx = o_pop(floor_ptr);
        if (i != o_idx)
            return 152;

        o_ptr = &floor_ptr->o_list[o_idx];
        rd_item(player_ptr, o_ptr);

        if (object_is_held_monster(o_ptr)) {
            monster_type *m_ptr;
            m_ptr = &floor_ptr->m_list[o_ptr->held_m_idx];
            o_ptr->next_o_idx = m_ptr->hold_o_idx;
            m_ptr->hold_o_idx = o_idx;
        } else {
            grid_type *g_ptr = &floor_ptr->grid_array[o_ptr->iy][o_ptr->ix];
            o_ptr->next_o_idx = g_ptr->o_idx;
            g_ptr->o_idx = o_idx;
        }
    }

    rd_u16b(&limit);
    if (limit > current_world_ptr->max_m_idx)
        return 161;

    for (int i = 1; i < limit; i++) {
        grid_type *g_ptr;
        MONSTER_IDX m_idx;
        monster_type *m_ptr;
        m_idx = m_pop(floor_ptr);
        if (i != m_idx)
            return 162;

        m_ptr = &floor_ptr->m_list[m_idx];
        rd_monster(player_ptr, m_ptr);
        g_ptr = &floor_ptr->grid_array[m_ptr->fy][m_ptr->fx];
        g_ptr->m_idx = m_idx;
        real_r_ptr(m_ptr)->cur_num++;
    }

    return 0;
}
