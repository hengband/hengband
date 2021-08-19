/*!
 * @file save.c
 * @brief セーブファイル書き込み処理 / Purpose: interact with savefiles
 * @date 2014/07/12
 * @author
 * <pre>
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 * </pre>
 */

#include "save/save.h"
#include "core/object-compressor.h"
#include "dungeon/quest.h"
#include "floor/floor-town.h"
#include "floor/wild.h"
#include "game-option/text-display-options.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/report.h"
#include "io/uid-checker.h"
#include "monster-race/monster-race.h"
#include "monster/monster-compaction.h"
#include "monster/monster-status.h"
#include "object/object-kind.h"
#include "player/player-status.h"
#include "save/floor-writer.h"
#include "save/info-writer.h"
#include "save/item-writer.h"
#include "save/monster-writer.h"
#include "save/player-writer.h"
#include "save/save-util.h"
#include "store/store-owners.h"
#include "store/store-util.h"
#include "system/angband-version.h"
#include "system/artifact-type-definition.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief セーブデータの書き込み /
 * Actually write a save-file
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 */
static bool wr_savefile_new(player_type *player_ptr, save_type type)
{
    compact_objects(player_ptr, 0);
    compact_monsters(player_ptr, 0);

    uint32_t now = (uint32_t)time((time_t *)0);
    current_world_ptr->sf_system = 0L;
    current_world_ptr->sf_when = now;
    current_world_ptr->sf_saves++;
    save_xor_byte = 0;
    wr_byte(FAKE_VER_MAJOR);
    save_xor_byte = 0;
    wr_byte(FAKE_VER_MINOR);
    save_xor_byte = 0;
    wr_byte(FAKE_VER_PATCH);
    save_xor_byte = 0;

    byte tmp8u = (byte)Rand_external(256);
    wr_byte(tmp8u);
    v_stamp = 0L;
    x_stamp = 0L;

    wr_byte(H_VER_EXTRA);
    wr_byte(H_VER_PATCH);
    wr_byte(H_VER_MINOR);
    wr_byte(H_VER_MAJOR);
    wr_u32b(current_world_ptr->sf_system);
    wr_u32b(current_world_ptr->sf_when);
    wr_u16b(current_world_ptr->sf_lives);
    wr_u16b(current_world_ptr->sf_saves);

    wr_u32b(SAVEFILE_VERSION);
    wr_u16b(0);
    wr_byte(0);

#ifdef JP
#ifdef EUC
    wr_byte(2);
#endif
#ifdef SJIS
    wr_byte(3);
#endif
#else
    wr_byte(1);
#endif

    wr_randomizer();
    wr_options(type);
    uint32_t tmp32u = message_num();
    if ((compress_savefile || (type == SAVE_TYPE_DEBUG)) && (tmp32u > 40))
        tmp32u = 40;

    wr_u32b(tmp32u);
    for (int i = tmp32u - 1; i >= 0; i--)
        wr_string(message_str(i));

    uint16_t tmp16u = max_r_idx;
    wr_u16b(tmp16u);
    for (MONRACE_IDX r_idx = 0; r_idx < tmp16u; r_idx++)
        wr_lore(r_idx);

    tmp16u = max_k_idx;
    wr_u16b(tmp16u);
    for (KIND_OBJECT_IDX k_idx = 0; k_idx < tmp16u; k_idx++)
        wr_perception(k_idx);

    tmp16u = max_towns;
    wr_u16b(tmp16u);

    tmp16u = max_q_idx;
    wr_u16b(tmp16u);

    tmp8u = MAX_RANDOM_QUEST - MIN_RANDOM_QUEST;
    wr_byte(tmp8u);

    for (int i = 0; i < max_q_idx; i++) {
        quest_type *const q_ptr = &quest[i];
        wr_s16b(q_ptr->status);
        wr_s16b((int16_t)q_ptr->level);
        wr_byte((byte)q_ptr->complev);
        wr_u32b(q_ptr->comptime);

        bool is_quest_running = q_ptr->status == QUEST_STATUS_TAKEN;
        is_quest_running |= q_ptr->status == QUEST_STATUS_COMPLETED;
        is_quest_running |= !is_fixed_quest_idx(i);
        if (!is_quest_running)
            continue;

        wr_s16b((int16_t)q_ptr->cur_num);
        wr_s16b((int16_t)q_ptr->max_num);
        wr_s16b(q_ptr->type);
        wr_s16b(q_ptr->r_idx);
        wr_s16b(q_ptr->k_idx);
        wr_byte((byte)q_ptr->flags);
        wr_byte((byte)q_ptr->dungeon);
    }

    wr_s32b(player_ptr->wilderness_x);
    wr_s32b(player_ptr->wilderness_y);
    wr_byte(player_ptr->wild_mode);
    wr_byte(player_ptr->ambush_flag);
    wr_s32b(current_world_ptr->max_wild_x);
    wr_s32b(current_world_ptr->max_wild_y);
    for (int i = 0; i < current_world_ptr->max_wild_x; i++)
        for (int j = 0; j < current_world_ptr->max_wild_y; j++)
            wr_u32b(wilderness[j][i].seed);

    tmp16u = max_a_idx;
    wr_u16b(tmp16u);
    for (int i = 0; i < tmp16u; i++) {
        artifact_type *a_ptr = &a_info[i];
        wr_byte(a_ptr->cur_num);
        wr_s16b(a_ptr->floor_id);
    }

    wr_u32b(current_world_ptr->sf_play_time);
    wr_FlagGroup(current_world_ptr->sf_winner, wr_byte);
    wr_FlagGroup(current_world_ptr->sf_retired, wr_byte);

    wr_player(player_ptr);
    tmp16u = PY_MAX_LEVEL;
    wr_u16b(tmp16u);
    for (int i = 0; i < tmp16u; i++)
        wr_s16b((int16_t)player_ptr->player_hp[i]);

    wr_u32b(player_ptr->spell_learned1);
    wr_u32b(player_ptr->spell_learned2);
    wr_u32b(player_ptr->spell_worked1);
    wr_u32b(player_ptr->spell_worked2);
    wr_u32b(player_ptr->spell_forgotten1);
    wr_u32b(player_ptr->spell_forgotten2);
    wr_s16b(player_ptr->learned_spells);
    wr_s16b(player_ptr->add_spells);
    for (int i = 0; i < 64; i++)
        wr_byte((byte)player_ptr->spell_order[i]);

    for (int i = 0; i < INVEN_TOTAL; i++) {
        object_type *o_ptr = &player_ptr->inventory_list[i];
        if (!o_ptr->k_idx)
            continue;

        wr_u16b((uint16_t)i);
        wr_item(o_ptr);
    }

    wr_u16b(0xFFFF);
    tmp16u = max_towns;
    wr_u16b(tmp16u);

    tmp16u = MAX_STORES;
    wr_u16b(tmp16u);
    for (int i = 1; i < max_towns; i++)
        for (int j = 0; j < MAX_STORES; j++)
            wr_store(&town_info[i].store[j]);

    wr_s16b(player_ptr->pet_follow_distance);
    wr_s16b(player_ptr->pet_extra_flags);
    if (screen_dump && (player_ptr->wait_report_score || !player_ptr->is_dead))
        wr_string(screen_dump);
    else
        wr_string("");

    if (!player_ptr->is_dead) {
        if (!wr_dungeon(player_ptr))
            return false;

        wr_ghost();
        wr_s32b(0);
    }

    wr_u32b(v_stamp);
    wr_u32b(x_stamp);
    return !ferror(saving_savefile) && (fflush(saving_savefile) != EOF);
}

/*!
 * @brief セーブデータ書き込みのサブルーチン /
 * Medium level player saver
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 * @details
 * Angband 2.8.0 will use "fd" instead of "fff" if possible
 */
static bool save_player_aux(player_type *player_ptr, char *name, save_type type)
{
    safe_setuid_grab(player_ptr);
    int file_permission = 0644;
    int fd = fd_make(name, file_permission);
    safe_setuid_drop();

    bool is_save_successful = false;
    saving_savefile = NULL;
    if (fd >= 0) {
        (void)fd_close(fd);
        safe_setuid_grab(player_ptr);
        saving_savefile = angband_fopen(name, "wb");
        safe_setuid_drop();
        if (saving_savefile) {
            if (wr_savefile_new(player_ptr, type))
                is_save_successful = true;

            if (angband_fclose(saving_savefile))
                is_save_successful = false;
        }

        safe_setuid_grab(player_ptr);
        if (!is_save_successful)
            (void)fd_kill(name);

        safe_setuid_drop();
    }

    if (!is_save_successful)
        return false;

    counts_write(player_ptr, 0, current_world_ptr->play_time);
    current_world_ptr->character_saved = true;
    return true;
}

/*!
 * @brief セーブデータ書き込みのメインルーチン /
 * Attempt to save the player in a savefile
 * @param player_ptr プレーヤーへの参照ポインタ
 * @return 成功すればtrue
 */
bool save_player(player_type *player_ptr, save_type type)
{
    char safe[1024];
    strcpy(safe, savefile);
    strcat(safe, ".new");
    safe_setuid_grab(player_ptr);
    fd_kill(safe);
    safe_setuid_drop();
    update_playtime();
    bool result = false;
    if (save_player_aux(player_ptr, safe, type)) {
        char temp[1024];
        char filename[1024];
        strcpy(temp, savefile);
        strcat(temp, ".old");
        safe_setuid_grab(player_ptr);
        fd_kill(temp);

        if (type == SAVE_TYPE_DEBUG)
            strcpy(filename, debug_savefile);
        if (type != SAVE_TYPE_DEBUG)
            strcpy(filename, savefile);

        fd_move(filename, temp);
        fd_move(safe, filename);
        fd_kill(temp);
        safe_setuid_drop();
        current_world_ptr->character_loaded = true;
        result = true;
    }

    if (type != SAVE_TYPE_CLOSE_GAME) {
        current_world_ptr->is_loading_now = false;
        update_creature(player_ptr);
        mproc_init(player_ptr->current_floor_ptr);
        current_world_ptr->is_loading_now = true;
    }

    return result;
}
