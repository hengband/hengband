#include "load/floor-loader.h"
#include "floor/floor-generator.h"
#include "floor/floor-object.h"
#include "floor/floor-save-util.h"
#include "game-option/birth-options.h"
#include "grid/feature.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "load/angband-version-comparer.h"
#include "load/item-loader.h"
#include "load/load-util.h"
#include "load/load-v1-5-0.h"
#include "load/monster-loader.h"
#include "load/old-feature-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "save/floor-writer.h"
#include "system/angband-version.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/object-type-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "world/world-object.h"
#include "world/world.h"

/*!
 * @brief 保存されたフロアを読み込む / Read the saved floor
 * @param player_ptr プレイヤーへの参照ポインタ
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
        int16_t tmp16s;
        rd_s16b(&tmp16s);
        floor_ptr->dun_level = (DEPTH)tmp16s;
        floor_ptr->base_level = floor_ptr->dun_level;
    } else {
        int16_t tmp16s;
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

        int32_t tmp32s;
        rd_s32b(&tmp32s);
        if (tmp32s != sf_ptr->last_visit)
            return 171;

        uint32_t tmp32u;
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

    int16_t tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->base_level = (DEPTH)tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->num_repro = (MONSTER_NUMBER)tmp16s;

    uint16_t tmp16u;
    rd_u16b(&tmp16u);
    player_ptr->y = (POSITION)tmp16u;

    rd_u16b(&tmp16u);
    player_ptr->x = (POSITION)tmp16u;

    rd_s16b(&tmp16s);
    floor_ptr->height = (POSITION)tmp16s;
    rd_s16b(&tmp16s);
    floor_ptr->width = (POSITION)tmp16s;

    rd_byte(&player_ptr->feeling);

    uint16_t limit;
    rd_u16b(&limit);
    C_MAKE(templates, limit, grid_template_type);

    for (int i = 0; i < limit; i++) {
        grid_template_type *ct_ptr = &templates[i];
        rd_u16b(&tmp16u);
        ct_ptr->info = (BIT_FLAGS)tmp16u;
        if (h_older_than(1, 7, 0, 2)) {
            byte tmp8u;
            rd_byte(&tmp8u);
            ct_ptr->feat = (int16_t)tmp8u;
            rd_byte(&tmp8u);
            ct_ptr->mimic = (int16_t)tmp8u;
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

        uint16_t id = 0;
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
    if (limit > w_ptr->max_o_idx)
        return 151;
    for (int i = 1; i < limit; i++) {
        OBJECT_IDX o_idx;
        object_type *o_ptr;
        o_idx = o_pop(floor_ptr);
        if (i != o_idx)
            return 152;

        o_ptr = &floor_ptr->o_list[o_idx];
        rd_item(o_ptr);

        auto &list = get_o_idx_list_contains(floor_ptr, o_idx);
        list.add(floor_ptr, o_idx, o_ptr->stack_idx);
    }

    rd_u16b(&limit);
    if (limit > w_ptr->max_m_idx)
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

/*!
 * @brief 保存フロア読み込みのサブ関数 / Actually load and verify a floor save data
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf_ptr 保存フロア読み込み先
 * @return 成功したらtrue
 */
static bool load_floor_aux(player_type *player_ptr, saved_floor_type *sf_ptr)
{
    uint32_t n_x_check, n_v_check;
    uint32_t o_x_check, o_v_check;

    load_xor_byte = 0;
    byte tmp8u;
    rd_byte(&tmp8u);

    v_check = 0L;
    x_check = 0L;

    w_ptr->h_ver_extra = H_VER_EXTRA;
    w_ptr->h_ver_patch = H_VER_PATCH;
    w_ptr->h_ver_minor = H_VER_MINOR;
    w_ptr->h_ver_major = H_VER_MAJOR;
    loading_savefile_version = SAVEFILE_VERSION;

    uint32_t tmp32u;
    rd_u32b(&tmp32u);
    if (saved_floor_file_sign != tmp32u)
        return false;

    if (rd_saved_floor(player_ptr, sf_ptr))
        return false;

    n_v_check = v_check;
    rd_u32b(&o_v_check);

    if (o_v_check != n_v_check)
        return false;

    n_x_check = x_check;
    rd_u32b(&o_x_check);

    if (o_x_check != n_x_check)
        return false;
    return true;
}

/*!
 * @brief 一時保存フロア情報を読み込む / Attempt to load the temporarily saved-floor data
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf_ptr 保存フロア読み込み先
 * @param mode オプション
 * @return 成功したらtrue
 */
bool load_floor(player_type *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode)
{
    /*
     * Temporary files are always written in system depended kanji
     * code.
     */
#ifdef JP
#ifdef EUC
    kanji_code = 2;
#endif
#ifdef SJIS
    kanji_code = 3;
#endif
#else
    kanji_code = 1;
#endif

    FILE *old_fff = nullptr;
    byte old_xor_byte = 0;
    uint32_t old_v_check = 0;
    uint32_t old_x_check = 0;
    byte old_h_ver_major = 0;
    byte old_h_ver_minor = 0;
    byte old_h_ver_patch = 0;
    byte old_h_ver_extra = 0;
    uint32_t old_loading_savefile_version = 0;
    if (mode & SLF_SECOND) {
        old_fff = loading_savefile;
        old_xor_byte = load_xor_byte;
        old_v_check = v_check;
        old_x_check = x_check;
        old_h_ver_major = w_ptr->h_ver_major;
        old_h_ver_minor = w_ptr->h_ver_minor;
        old_h_ver_patch = w_ptr->h_ver_patch;
        old_h_ver_extra = w_ptr->h_ver_extra;
        old_loading_savefile_version = loading_savefile_version;
    }

    char floor_savefile[sizeof(savefile) + 32];
    sprintf(floor_savefile, "%s.F%02d", savefile, (int)sf_ptr->savefile_id);

    safe_setuid_grab(player_ptr);
    loading_savefile = angband_fopen(floor_savefile, "rb");
    safe_setuid_drop();

    bool is_save_successful = true;
    if (!loading_savefile)
        is_save_successful = false;

    if (is_save_successful) {
        is_save_successful = load_floor_aux(player_ptr, sf_ptr);
        if (ferror(loading_savefile))
            is_save_successful = false;

        angband_fclose(loading_savefile);
        safe_setuid_grab(player_ptr);
        if (!(mode & SLF_NO_KILL))
            (void)fd_kill(floor_savefile);

        safe_setuid_drop();
    }

    if (mode & SLF_SECOND) {
        loading_savefile = old_fff;
        load_xor_byte = old_xor_byte;
        v_check = old_v_check;
        x_check = old_x_check;
        w_ptr->h_ver_major = old_h_ver_major;
        w_ptr->h_ver_minor = old_h_ver_minor;
        w_ptr->h_ver_patch = old_h_ver_patch;
        w_ptr->h_ver_extra = old_h_ver_extra;
        loading_savefile_version = old_loading_savefile_version;
    }

    byte old_kanji_code = kanji_code;
    kanji_code = old_kanji_code;
    return is_save_successful;
}
