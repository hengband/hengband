#include "save/floor-writer.h"
#include "core/object-compressor.h"
#include "core/player-update-types.h"
#include "floor/floor-events.h"
#include "floor/floor-save-util.h"
#include "floor/floor-save.h"
#include "grid/grid.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "load/floor-loader.h"
#include "monster-floor/monster-lite.h"
#include "monster/monster-compaction.h"
#include "save/item-writer.h"
#include "save/monster-writer.h"
#include "save/save-util.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "util/sort.h"

/*!
 * @brief 保存フロアの書き込み / Actually write a saved floor data using effectively compressed format.
 * @param sf_ptr 保存したいフロアの参照ポインタ
 */
void wr_saved_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!sf_ptr) {
        wr_s16b((int16_t)floor_ptr->dun_level);
    } else {
        wr_s16b(sf_ptr->floor_id);
        wr_byte((byte)sf_ptr->savefile_id);
        wr_s16b((int16_t)sf_ptr->dun_level);
        wr_s32b(sf_ptr->last_visit);
        wr_u32b(sf_ptr->visit_mark);
        wr_s16b(sf_ptr->upper_floor_id);
        wr_s16b(sf_ptr->lower_floor_id);
    }

    wr_u16b((uint16_t)floor_ptr->base_level);
    wr_u16b((int16_t)player_ptr->current_floor_ptr->num_repro);
    wr_u16b((uint16_t)player_ptr->y);
    wr_u16b((uint16_t)player_ptr->x);
    wr_u16b((uint16_t)floor_ptr->height);
    wr_u16b((uint16_t)floor_ptr->width);
    wr_byte(player_ptr->feeling);

    /*
     * Usually number of templates are fewer than 255.  Even if
     * more than 254 are exist, the occurrence of each template
     * with larger ID is very small when we sort templates by
     * occurrence.  So we will use two (or more) bytes for
     * templete ID larger than 254.
     *
     * Ex: 256 will be "0xff" "0x01".
     *     515 will be "0xff" "0xff" "0x03"
     */

    std::vector<grid_template_type> templates;
    for (int y = 0; y < floor_ptr->height; y++) {
        for (int x = 0; x < floor_ptr->width; x++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            uint i;
            for (i = 0; i < templates.size(); i++) {
                if (templates[i].info == g_ptr->info && templates[i].feat == g_ptr->feat && templates[i].mimic == g_ptr->mimic && templates[i].special == g_ptr->special) {
                    templates[i].occurrence++;
                    break;
                }
            }

            if (i < templates.size()) {
                continue;
            }

            templates.push_back({ g_ptr->info, g_ptr->feat, g_ptr->mimic, g_ptr->special, 1 });
        }
    }

    int dummy_why;
    ang_sort(player_ptr, templates.data(), &dummy_why, templates.size(), ang_sort_comp_cave_temp, ang_sort_swap_cave_temp);

    /*** Dump templates ***/
    wr_u16b(static_cast<uint16_t>(templates.size()));
    for (const auto &ct_ref : templates) {
        wr_u16b(static_cast<uint16_t>(ct_ref.info));
        wr_s16b(ct_ref.feat);
        wr_s16b(ct_ref.mimic);
        wr_s16b(ct_ref.special);
    }

    byte count = 0;
    uint16_t prev_u16b = 0;
    for (int y = 0; y < floor_ptr->height; y++) {
        for (int x = 0; x < floor_ptr->width; x++) {
            auto *g_ptr = &floor_ptr->grid_array[y][x];
            uint i;
            for (i = 0; i < templates.size(); i++) {
                if (templates[i].info == g_ptr->info && templates[i].feat == g_ptr->feat && templates[i].mimic == g_ptr->mimic && templates[i].special == g_ptr->special) {
                    break;
                }
            }

            uint16_t tmp16u = (uint16_t)i;
            if ((tmp16u == prev_u16b) && (count != MAX_UCHAR)) {
                count++;
                continue;
            }

            wr_byte((byte)count);
            while (prev_u16b >= MAX_UCHAR) {
                wr_byte(MAX_UCHAR);
                prev_u16b -= MAX_UCHAR;
            }

            wr_byte((byte)prev_u16b);
            prev_u16b = tmp16u;
            count = 1;
        }
    }

    if (count > 0) {
        wr_byte((byte)count);
        while (prev_u16b >= MAX_UCHAR) {
            wr_byte(MAX_UCHAR);
            prev_u16b -= MAX_UCHAR;
        }

        wr_byte((byte)prev_u16b);
    }

    /*** Dump objects ***/
    wr_u16b(floor_ptr->o_max);
    for (int i = 1; i < floor_ptr->o_max; i++) {
        auto *o_ptr = &floor_ptr->o_list[i];
        wr_item(o_ptr);
    }

    /*** Dump the monsters ***/
    wr_u16b(floor_ptr->m_max);
    for (int i = 1; i < floor_ptr->m_max; i++) {
        auto *m_ptr = &floor_ptr->m_list[i];
        wr_monster(m_ptr);
    }
}

/*!
 * @brief 現在フロアの書き込み /
 * Write the current dungeon (new method)
 * @player_ptr プレイヤーへの参照ポインタ
 * @return 保存に成功したらTRUE
 */
bool wr_dungeon(PlayerType *player_ptr)
{
    forget_lite(player_ptr->current_floor_ptr);
    forget_view(player_ptr->current_floor_ptr);
    clear_mon_lite(player_ptr->current_floor_ptr);
    player_ptr->update |= PU_VIEW | PU_LITE | PU_MON_LITE;
    player_ptr->update |= PU_MONSTERS | PU_DISTANCE | PU_FLOW;
    wr_s16b(max_floor_id);
    wr_byte((byte)player_ptr->dungeon_idx);
    if (!player_ptr->floor_id) {
        /* No array elements */
        wr_byte(0);
        wr_saved_floor(player_ptr, nullptr);
        return true;
    }

    /*** In the dungeon ***/
    wr_byte(MAX_SAVED_FLOORS);
    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        wr_s16b(sf_ptr->floor_id);
        wr_byte((byte)sf_ptr->savefile_id);
        wr_s16b((int16_t)sf_ptr->dun_level);
        wr_s32b(sf_ptr->last_visit);
        wr_u32b(sf_ptr->visit_mark);
        wr_s16b(sf_ptr->upper_floor_id);
        wr_s16b(sf_ptr->lower_floor_id);
    }

    saved_floor_type *cur_sf_ptr;
    cur_sf_ptr = get_sf_ptr(player_ptr->floor_id);
    if (!save_floor(player_ptr, cur_sf_ptr, SLF_SECOND)) {
        return false;
    }

    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        if (!sf_ptr->floor_id) {
            continue;
        }
        if (!load_floor(player_ptr, sf_ptr, (SLF_SECOND | SLF_NO_KILL))) {
            wr_byte(1);
            continue;
        }

        wr_byte(0);
        wr_saved_floor(player_ptr, sf_ptr);
    }

    return load_floor(player_ptr, cur_sf_ptr, (SLF_SECOND));
}

/*!
 * @brief ゲームプレイ中のフロア一時保存出力処理サブルーチン / Actually write a temporary saved floor file
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf_ptr 保存フロア参照ポインタ
 */
static bool save_floor_aux(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    compact_objects(player_ptr, 0);
    compact_monsters(player_ptr, 0);

    byte tmp8u = (byte)randint0(256);
    save_xor_byte = 0;
    wr_byte(tmp8u);

    /* Reset the checksum */
    v_stamp = 0L;
    x_stamp = 0L;
    wr_u32b(saved_floor_file_sign);
    wr_saved_floor(player_ptr, sf_ptr);
    wr_u32b(v_stamp);
    wr_u32b(x_stamp);

    return !ferror(saving_savefile) && (fflush(saving_savefile) != EOF);
}
/*!
 * @brief ゲームプレイ中のフロア一時保存出力処理メインルーチン / Attempt to save the temporarily saved-floor data
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf_ptr 保存フロア参照ポインタ
 * @param mode 保存オプション
 */
bool save_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr, BIT_FLAGS mode)
{
    FILE *old_fff = nullptr;
    byte old_xor_byte = 0;
    uint32_t old_v_stamp = 0;
    uint32_t old_x_stamp = 0;

    if ((mode & SLF_SECOND) != 0) {
        old_fff = saving_savefile;
        old_xor_byte = save_xor_byte;
        old_v_stamp = v_stamp;
        old_x_stamp = x_stamp;
    }

    std::string floor_savefile = savefile;
    char ext[32];
    strnfmt(ext, sizeof(ext), ".F%02d", (int)sf_ptr->savefile_id);
    floor_savefile.append(ext);
    safe_setuid_grab(player_ptr);
    fd_kill(floor_savefile.data());
    safe_setuid_drop();
    saving_savefile = nullptr;
    safe_setuid_grab(player_ptr);

    int fd = fd_make(floor_savefile.data(), 0644);
    safe_setuid_drop();
    bool is_save_successful = false;
    if (fd >= 0) {
        (void)fd_close(fd);
        safe_setuid_grab(player_ptr);
        saving_savefile = angband_fopen(floor_savefile.data(), "wb");
        safe_setuid_drop();
        if (saving_savefile) {
            if (save_floor_aux(player_ptr, sf_ptr)) {
                is_save_successful = true;
            }

            if (angband_fclose(saving_savefile)) {
                is_save_successful = false;
            }
        }

        if (!is_save_successful) {
            safe_setuid_grab(player_ptr);
            (void)fd_kill(floor_savefile.data());
            safe_setuid_drop();
        }
    }

    if ((mode & SLF_SECOND) != 0) {
        saving_savefile = old_fff;
        save_xor_byte = old_xor_byte;
        v_stamp = old_v_stamp;
        x_stamp = old_x_stamp;
    }

    return is_save_successful;
}
