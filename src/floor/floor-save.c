/*!
 * @brief 保存された階の管理 / management of the saved floor
 * @date 2014/01/04
 * @author
 * Copyright (c) 2002  Mogami \n
 * This software may be copied and distributed for educational, research, and \n
 * not for profit purposes provided that this copyright and statement are \n
 * included in all such copies. \n
 * 2014 Deskull rearranged comment for Doxygen. \n
 */

#include "floor/floor-save.h"
#include "action/travel-execution.h"
#include "cmd-building/cmd-building.h"
#include "cmd-io/cmd-dump.h"
#include "core/asking-player.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest-monster-placer.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-changer.h"
#include "floor/floor-events.h"
#include "floor/floor-generator.h"
#include "floor/floor-mode-changer.h"
#include "floor/floor-object.h"
#include "floor/floor-save-util.h"
#include "floor/floor.h"
#include "floor/geometry.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "grid/grid.h"
#include "inventory/inventory-slot-types.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "io/write-diary.h"
#include "load/floor-loader.h"
#include "main/sound-of-music.h"
#include "mind/mind-mirror-master.h"
#include "mind/mind-ninja.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/monster-lite.h"
#include "monster-floor/monster-remover.h"
#include "monster-floor/monster-summon.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags7.h"
#include "monster/monster-describer.h"
#include "monster/monster-description-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-list.h"
#include "monster/monster-status-setter.h"
#include "monster/monster-status.h"
#include "monster/monster-update.h"
#include "monster/smart-learn-types.h"
#include "object-hook/hook-checker.h"
#include "object-hook/hook-enchant.h"
#include "pet/pet-util.h"
#include "player/player-class.h"
#include "player/player-personalities-types.h"
#include "player/special-defense-types.h"
#include "save/floor-writer.h"
#include "spell-kind/spells-floor.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/system-variables.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "world/world.h"

/*!
 * @brief 保存フロア配列を初期化する / Initialize saved_floors array.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param force テンポラリファイルが残っていた場合も警告なしで強制的に削除するフラグ
 * @details Make sure that old temporary files are not remaining as gurbages.
 * @return なし
 */
void init_saved_floors(player_type *creature_ptr, bool force)
{
    char floor_savefile[1024];
    int fd = -1;
    BIT_FLAGS mode = 0644;
    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        sprintf(floor_savefile, "%s.F%02d", savefile, i);
        safe_setuid_grab(creature_ptr);
        fd = fd_make(floor_savefile, mode);
        safe_setuid_drop();
        if (fd < 0) {
            if (!force) {
                msg_print(_("エラー：古いテンポラリ・ファイルが残っています。", "Error: There are old temporary files."));
                msg_print(_("変愚蛮怒を二重に起動していないか確認してください。", "Make sure you are not running two game processes simultaneously."));
                msg_print(_("過去に変愚蛮怒がクラッシュした場合は一時ファイルを", "If the temporary files are garbage from an old crashed process, "));
                msg_print(_("強制的に削除して実行を続けられます。", "you can delete them safely."));
                if (!get_check(_("強制的に削除してもよろしいですか？", "Do you delete the old temporary files? ")))
                    quit(_("実行中止", "Aborted."));

                force = TRUE;
            }
        } else
            (void)fd_close(fd);

        safe_setuid_grab(creature_ptr);
        (void)fd_kill(floor_savefile);
        safe_setuid_drop();
        sf_ptr->floor_id = 0;
    }

    max_floor_id = 1;
    latest_visit_mark = 1;
    saved_floor_file_sign = (u32b)time(NULL);
    new_floor_id = 0;
    creature_ptr->change_floor_mode = 0;
}

/*!
 * @brief 保存フロア用テンポラリファイルを削除する / Kill temporary files
 * @details Should be called just before the game quit.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void clear_saved_floor_files(player_type *creature_ptr)
{
    char floor_savefile[1024];
    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        if ((sf_ptr->floor_id == 0) || (sf_ptr->floor_id == creature_ptr->floor_id))
            continue;

        sprintf(floor_savefile, "%s.F%02d", savefile, i);
        safe_setuid_grab(creature_ptr);
        (void)fd_kill(floor_savefile);
        safe_setuid_drop();
    }
}

/*!
 * @brief 保存フロアIDから参照ポインタを得る / Get a pointer for an item of the saved_floors array.
 * @param floor_id 保存フロアID
 * @return IDに対応する保存フロアのポインタ、ない場合はNULLを返す。
 */
saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id)
{
    if (!floor_id)
        return NULL;

    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        if (sf_ptr->floor_id == floor_id)
            return sf_ptr;
    }

    return NULL;
}

/*!
 * @brief 参照ポインタ先の保存フロアを抹消する / kill a saved floor and get an empty space
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @param sf_ptr 保存フロアの参照ポインタ
 * @return なし
 */
static void kill_saved_floor(player_type *creature_ptr, saved_floor_type *sf_ptr)
{
    char floor_savefile[1024];
    if (!sf_ptr || (sf_ptr->floor_id == 0))
        return;

    if (sf_ptr->floor_id == creature_ptr->floor_id) {
        creature_ptr->floor_id = 0;
        sf_ptr->floor_id = 0;
        return;
    }

    sprintf(floor_savefile, "%s.F%02d", savefile, (int)sf_ptr->savefile_id);
    safe_setuid_grab(creature_ptr);
    (void)fd_kill(floor_savefile);
    safe_setuid_drop();
    sf_ptr->floor_id = 0;
}

/*!
 * @brief 新規に利用可能な保存フロアを返す / Initialize new saved floor and get its floor id.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return 利用可能な保存フロアID
 * @details
 * If number of saved floors are already MAX_SAVED_FLOORS, kill the oldest one.
 */
FLOOR_IDX get_new_floor_id(player_type *creature_ptr)
{
    saved_floor_type *sf_ptr = NULL;
    FLOOR_IDX i;
    for (i = 0; i < MAX_SAVED_FLOORS; i++) {
        sf_ptr = &saved_floors[i];
        if (!sf_ptr->floor_id)
            break;
    }

    if (i == MAX_SAVED_FLOORS) {
        s16b oldest = 0;
        u32b oldest_visit = 0xffffffffL;
        for (i = 0; i < MAX_SAVED_FLOORS; i++) {
            sf_ptr = &saved_floors[i];
            if ((sf_ptr->floor_id == creature_ptr->floor_id) || (sf_ptr->visit_mark > oldest_visit))
                continue;

            oldest = i;
            oldest_visit = sf_ptr->visit_mark;
        }

        sf_ptr = &saved_floors[oldest];
        kill_saved_floor(creature_ptr, sf_ptr);
        i = oldest;
    }

    sf_ptr->savefile_id = i;
    sf_ptr->floor_id = max_floor_id;
    sf_ptr->last_visit = 0;
    sf_ptr->upper_floor_id = 0;
    sf_ptr->lower_floor_id = 0;
    sf_ptr->visit_mark = latest_visit_mark++;
    sf_ptr->dun_level = creature_ptr->current_floor_ptr->dun_level;
    if (max_floor_id < MAX_SHORT)
        max_floor_id++;
    else
        max_floor_id = 1; // 32767 floor_ids are all used up!  Re-use ancient IDs.

    return sf_ptr->floor_id;
}

/*!
 * @brief フロア移動時にペットを伴った場合の準備処理 / Pre-calculate the racial counters of preserved pets
 * @param master_ptr プレーヤーへの参照ポインタ
 * @return なし
 * @details
 * To prevent multiple generation of unique monster who is the minion of player
 */
void precalc_cur_num_of_pet(player_type *player_ptr)
{
    monster_type *m_ptr;
    int max_num = player_ptr->wild_mode ? 1 : MAX_PARTY_MON;
    for (int i = 0; i < max_num; i++) {
        m_ptr = &party_mon[i];
        if (!monster_is_valid(m_ptr))
            continue;

        real_r_ptr(m_ptr)->cur_num++;
    }
}

/*!
 * @brief フロア移動時のペット保存処理 / Preserve_pets
 * @param master_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
static void preserve_pet(player_type *master_ptr)
{
    for (MONSTER_IDX party_monster_num = 0; party_monster_num < MAX_PARTY_MON; party_monster_num++)
        party_mon[party_monster_num].r_idx = 0;

    if (master_ptr->riding) {
        monster_type *m_ptr = &master_ptr->current_floor_ptr->m_list[master_ptr->riding];
        if (m_ptr->parent_m_idx) {
            master_ptr->riding = 0;
            master_ptr->pet_extra_flags &= ~(PF_TWO_HANDS);
            master_ptr->riding_ryoute = master_ptr->old_riding_ryoute = FALSE;
        } else {
            (void)COPY(&party_mon[0], m_ptr, monster_type);
            delete_monster_idx(master_ptr, master_ptr->riding);
        }
    }

    /*
     * todo 関数分割時の参考とするため、このコメントは残しておく。分割後はnumをparty_monster_numに変更する.
     * If player is in wild mode, no pets are preserved except a monster whom player riding.
     */
    if (!master_ptr->wild_mode && !master_ptr->current_floor_ptr->inside_arena && !master_ptr->phase_out) {
        for (MONSTER_IDX i = master_ptr->current_floor_ptr->m_max - 1, num = 1; (i >= 1 && num < MAX_PARTY_MON); i--) {
            monster_type *m_ptr = &master_ptr->current_floor_ptr->m_list[i];
            if (!monster_is_valid(m_ptr) || !is_pet(m_ptr) || (i == master_ptr->riding))
                continue;

            if (reinit_wilderness) {
            } else {
                POSITION dis = distance(master_ptr->y, master_ptr->x, m_ptr->fy, m_ptr->fx);
                if (monster_confused_remaining(m_ptr) || monster_stunned_remaining(m_ptr) || monster_csleep_remaining(m_ptr) || (m_ptr->parent_m_idx != 0))
                    continue;

                if (m_ptr->nickname
                    && ((player_has_los_bold(master_ptr, m_ptr->fy, m_ptr->fx) && projectable(master_ptr, master_ptr->y, master_ptr->x, m_ptr->fy, m_ptr->fx))
                        || (los(master_ptr, m_ptr->fy, m_ptr->fx, master_ptr->y, master_ptr->x)
                            && projectable(master_ptr, m_ptr->fy, m_ptr->fx, master_ptr->y, master_ptr->x)))) {
                    if (dis > 3)
                        continue;
                } else if (dis > 1)
                    continue;
            }

            (void)COPY(&party_mon[num], &master_ptr->current_floor_ptr->m_list[i], monster_type);
            num++;
            delete_monster_idx(master_ptr, i);
        }
    }

    if (record_named_pet) {
        for (MONSTER_IDX i = master_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
            monster_type *m_ptr = &master_ptr->current_floor_ptr->m_list[i];
            GAME_TEXT m_name[MAX_NLEN];
            if (!monster_is_valid(m_ptr) || !is_pet(m_ptr) || !m_ptr->nickname || (master_ptr->riding == i))
                continue;

            monster_desc(master_ptr, m_name, m_ptr, MD_ASSUME_VISIBLE | MD_INDEF_VISIBLE);
            exe_write_diary(master_ptr, DIARY_NAMED_PET, RECORD_NAMED_PET_MOVED, m_name);
        }
    }

    for (MONSTER_IDX i = master_ptr->current_floor_ptr->m_max - 1; i >= 1; i--) {
        monster_type *m_ptr = &master_ptr->current_floor_ptr->m_list[i];
        if ((m_ptr->parent_m_idx == 0) || (master_ptr->current_floor_ptr->m_list[m_ptr->parent_m_idx].r_idx != 0))
            continue;

        if (is_seen(master_ptr, m_ptr)) {
            GAME_TEXT m_name[MAX_NLEN];
            monster_desc(master_ptr, m_name, m_ptr, 0);
            msg_format(_("%sは消え去った！", "%^s disappears!"), m_name);
        }

        delete_monster_idx(master_ptr, i);
    }
}

/*!
 * @brief 新フロアに移動元フロアに繋がる階段を配置する / Virtually teleport onto the stairs that is connecting between two floors.
 * @param sf_ptr 移動元の保存フロア構造体参照ポインタ
 * @return なし
 */
static void locate_connected_stairs(player_type *creature_ptr, floor_type *floor_ptr, saved_floor_type *sf_ptr, BIT_FLAGS floor_mode)
{
    POSITION sx = 0;
    POSITION sy = 0;
    POSITION x_table[20];
    POSITION y_table[20];
    int num = 0;
    for (POSITION y = 0; y < floor_ptr->height; y++) {
        for (POSITION x = 0; x < floor_ptr->width; x++) {
            grid_type *g_ptr = &floor_ptr->grid_array[y][x];
            feature_type *f_ptr = &f_info[g_ptr->feat];
            bool ok = FALSE;
            if (floor_mode & CFM_UP) {
                if (have_flag(f_ptr->flags, FF_LESS) && have_flag(f_ptr->flags, FF_STAIRS) && !have_flag(f_ptr->flags, FF_SPECIAL)) {
                    ok = TRUE;
                    if (g_ptr->special && g_ptr->special == sf_ptr->upper_floor_id) {
                        sx = x;
                        sy = y;
                    }
                }
            } else if (floor_mode & CFM_DOWN) {
                if (have_flag(f_ptr->flags, FF_MORE) && have_flag(f_ptr->flags, FF_STAIRS) && !have_flag(f_ptr->flags, FF_SPECIAL)) {
                    ok = TRUE;
                    if (g_ptr->special && g_ptr->special == sf_ptr->lower_floor_id) {
                        sx = x;
                        sy = y;
                    }
                }
            } else {
                if (have_flag(f_ptr->flags, FF_BLDG)) {
                    ok = TRUE;
                }
            }

            if (ok && (num < 20)) {
                x_table[num] = x;
                y_table[num] = y;
                num++;
            }
        }
    }

    if (sx) {
        creature_ptr->y = sy;
        creature_ptr->x = sx;
        return;
    }

    if (num == 0) {
        prepare_change_floor_mode(creature_ptr, CFM_RAND_PLACE | CFM_NO_RETURN);
        if (!feat_uses_special(floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].feat))
            floor_ptr->grid_array[creature_ptr->y][creature_ptr->x].special = 0;

        return;
    }

    int i = randint0(num);
    creature_ptr->y = y_table[i];
    creature_ptr->x = x_table[i];
}

/*!
 * @brief フロア移動時、プレイヤーの移動先モンスターが既にいた場合ランダムな近隣に移動させる / When a monster is at a place where player will return,
 * @return なし
 */
static void get_out_monster(player_type *protected_ptr)
{
    int tries = 0;
    POSITION dis = 1;
    POSITION oy = protected_ptr->y;
    POSITION ox = protected_ptr->x;
    floor_type *floor_ptr = protected_ptr->current_floor_ptr;
    MONSTER_IDX m_idx = floor_ptr->grid_array[oy][ox].m_idx;
    if (m_idx == 0)
        return;

    while (TRUE) {
        monster_type *m_ptr;
        POSITION ny = rand_spread(oy, dis);
        POSITION nx = rand_spread(ox, dis);
        tries++;
        if (tries > 10000)
            return;

        if (tries > 20 * dis * dis)
            dis++;

        if (!in_bounds(floor_ptr, ny, nx) || !is_cave_empty_bold(protected_ptr, ny, nx) || is_glyph_grid(&floor_ptr->grid_array[ny][nx])
            || is_explosive_rune_grid(&floor_ptr->grid_array[ny][nx]) || pattern_tile(floor_ptr, ny, nx))
            continue;

        m_ptr = &floor_ptr->m_list[m_idx];
        floor_ptr->grid_array[oy][ox].m_idx = 0;
        floor_ptr->grid_array[ny][nx].m_idx = m_idx;
        m_ptr->fy = ny;
        m_ptr->fx = nx;
        return;
    }
}

/*!
 * @brief 現在のフロアを離れるに伴って行なわれる保存処理
 * / Maintain quest monsters, mark next floor_id at stairs, save current floor, and prepare to enter next floor.
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @return なし
 */
void leave_floor(player_type *creature_ptr)
{
    grid_type *g_ptr = NULL;
    feature_type *f_ptr;
    saved_floor_type *sf_ptr;
    MONRACE_IDX quest_r_idx = 0;
    FLOOR_IDX tmp_floor_idx = 0;
    preserve_pet(creature_ptr);
    remove_all_mirrors(creature_ptr, FALSE);
    if (creature_ptr->special_defense & NINJA_S_STEALTH)
        set_superstealth(creature_ptr, FALSE);

    new_floor_id = 0;
    if (!creature_ptr->floor_id && (creature_ptr->change_floor_mode & CFM_SAVE_FLOORS) && !(creature_ptr->change_floor_mode & CFM_NO_RETURN))
        tmp_floor_idx = get_new_floor_id(creature_ptr);

    for (DUNGEON_IDX i = 0; i < max_q_idx; i++) {
        if ((quest[i].status == QUEST_STATUS_TAKEN) && ((quest[i].type == QUEST_TYPE_KILL_LEVEL) || (quest[i].type == QUEST_TYPE_RANDOM))
            && (quest[i].level == creature_ptr->current_floor_ptr->dun_level) && (creature_ptr->dungeon_idx == quest[i].dungeon)
            && !(quest[i].flags & QUEST_FLAG_PRESET)) {
            quest_r_idx = quest[i].r_idx;
        }
    }

    for (DUNGEON_IDX i = 1; i < creature_ptr->current_floor_ptr->m_max; i++) {
        monster_race *r_ptr;
        monster_type *m_ptr = &creature_ptr->current_floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr) || (quest_r_idx != m_ptr->r_idx))
            continue;

        r_ptr = real_r_ptr(m_ptr);
        if ((r_ptr->flags1 & RF1_UNIQUE) || (r_ptr->flags7 & RF7_NAZGUL))
            continue;

        delete_monster_idx(creature_ptr, i);
    }

    for (DUNGEON_IDX i = 0; i < INVEN_PACK; i++) {
        object_type *o_ptr = &creature_ptr->inventory_list[i];
        if (!object_is_valid(o_ptr))
            continue;

        if (object_is_fixed_artifact(o_ptr))
            a_info[o_ptr->name1].floor_id = 0;
    }

    sf_ptr = get_sf_ptr(creature_ptr->floor_id);
    if ((creature_ptr->change_floor_mode & CFM_RAND_CONNECT) && tmp_floor_idx)
        locate_connected_stairs(creature_ptr, creature_ptr->current_floor_ptr, sf_ptr, creature_ptr->change_floor_mode);

    if (creature_ptr->change_floor_mode & CFM_SAVE_FLOORS) {
        g_ptr = &creature_ptr->current_floor_ptr->grid_array[creature_ptr->y][creature_ptr->x];
        f_ptr = &f_info[g_ptr->feat];
        if (g_ptr->special && !have_flag(f_ptr->flags, FF_SPECIAL) && get_sf_ptr(g_ptr->special))
            new_floor_id = g_ptr->special;

        if (have_flag(f_ptr->flags, FF_STAIRS) && have_flag(f_ptr->flags, FF_SHAFT))
            prepare_change_floor_mode(creature_ptr, CFM_SHAFT);
    }

    if (creature_ptr->change_floor_mode & (CFM_DOWN | CFM_UP)) {
        int move_num = 0;
        if (creature_ptr->change_floor_mode & CFM_DOWN)
            move_num = 1;
        else if (creature_ptr->change_floor_mode & CFM_UP)
            move_num = -1;

        if (creature_ptr->change_floor_mode & CFM_SHAFT)
            move_num += SGN(move_num);

        if (creature_ptr->change_floor_mode & CFM_DOWN) {
            if (!creature_ptr->current_floor_ptr->dun_level)
                move_num = d_info[creature_ptr->dungeon_idx].mindepth;
        } else if (creature_ptr->change_floor_mode & CFM_UP) {
            if (creature_ptr->current_floor_ptr->dun_level + move_num < d_info[creature_ptr->dungeon_idx].mindepth)
                move_num = -creature_ptr->current_floor_ptr->dun_level;
        }

        creature_ptr->current_floor_ptr->dun_level += move_num;
    }

    if (!creature_ptr->current_floor_ptr->dun_level && creature_ptr->dungeon_idx) {
        creature_ptr->leaving_dungeon = TRUE;
        if (!vanilla_town && !lite_town) {
            creature_ptr->wilderness_y = d_info[creature_ptr->dungeon_idx].dy;
            creature_ptr->wilderness_x = d_info[creature_ptr->dungeon_idx].dx;
        }

        creature_ptr->recall_dungeon = creature_ptr->dungeon_idx;
        creature_ptr->dungeon_idx = 0;
        creature_ptr->change_floor_mode &= ~CFM_SAVE_FLOORS; // TODO
    }

    if (!(creature_ptr->change_floor_mode & CFM_SAVE_FLOORS)) {
        for (DUNGEON_IDX i = 0; i < MAX_SAVED_FLOORS; i++)
            kill_saved_floor(creature_ptr, &saved_floors[i]);

        latest_visit_mark = 1;
    } else if (creature_ptr->change_floor_mode & CFM_NO_RETURN) {
        kill_saved_floor(creature_ptr, sf_ptr);
    }

    if (creature_ptr->floor_id == 0)
        return;

    if (new_floor_id == 0) {
        new_floor_id = get_new_floor_id(creature_ptr);
        if (g_ptr && !feat_uses_special(g_ptr->feat))
            g_ptr->special = new_floor_id;
    }

    if (creature_ptr->change_floor_mode & CFM_RAND_CONNECT) {
        if (creature_ptr->change_floor_mode & CFM_UP)
            sf_ptr->upper_floor_id = new_floor_id;
        else if (creature_ptr->change_floor_mode & CFM_DOWN)
            sf_ptr->lower_floor_id = new_floor_id;
    }

    if (((creature_ptr->change_floor_mode & CFM_SAVE_FLOORS) == 0) || ((creature_ptr->change_floor_mode & CFM_NO_RETURN) != 0))
        return;

    get_out_monster(creature_ptr);
    sf_ptr->last_visit = current_world_ptr->game_turn;
    forget_lite(creature_ptr->current_floor_ptr);
    forget_view(creature_ptr->current_floor_ptr);
    clear_mon_lite(creature_ptr->current_floor_ptr);
    if (!save_floor(creature_ptr, sf_ptr, 0)) {
        prepare_change_floor_mode(creature_ptr, CFM_NO_RETURN);
        kill_saved_floor(creature_ptr, get_sf_ptr(creature_ptr->floor_id));
    }
}
