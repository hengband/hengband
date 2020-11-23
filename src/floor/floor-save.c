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
#include "core/asking-player.h"
#include "floor/floor-save-util.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "util/angband-files.h"
#include "view/display-messages.h"

static void check_saved_tmp_files(const int fd, bool *force)
{
    if (fd >= 0) {
        (void)fd_close(fd);
        return;
    }
    
    if (*force)
        return;

    msg_print(_("エラー：古いテンポラリ・ファイルが残っています。", "Error: There are old temporary files."));
    msg_print(_("変愚蛮怒を二重に起動していないか確認してください。", "Make sure you are not running two game processes simultaneously."));
    msg_print(_("過去に変愚蛮怒がクラッシュした場合は一時ファイルを", "If the temporary files are garbage from an old crashed process, "));
    msg_print(_("強制的に削除して実行を続けられます。", "you can delete them safely."));
    if (!get_check(_("強制的に削除してもよろしいですか？", "Do you delete the old temporary files? ")))
        quit(_("実行中止", "Aborted."));

    *force = TRUE;
}

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
        check_saved_tmp_files(fd, &force);
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
void kill_saved_floor(player_type *creature_ptr, saved_floor_type *sf_ptr)
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

static void find_oldest_floor_id(player_type *creature_ptr, saved_floor_type *sf_ptr, FLOOR_IDX *fl_idx)
{
    if (*fl_idx != MAX_SAVED_FLOORS)
        return;

    s16b oldest = 0;
    u32b oldest_visit = 0xffffffffL;
    for (*fl_idx = 0; *fl_idx < MAX_SAVED_FLOORS; (*fl_idx)++) {
        sf_ptr = &saved_floors[*fl_idx];
        if ((sf_ptr->floor_id == creature_ptr->floor_id) || (sf_ptr->visit_mark > oldest_visit))
            continue;

        oldest = *fl_idx;
        oldest_visit = sf_ptr->visit_mark;
    }

    sf_ptr = &saved_floors[oldest];
    kill_saved_floor(creature_ptr, sf_ptr);
    *fl_idx = oldest;
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
    FLOOR_IDX fl_idx;
    for (fl_idx = 0; fl_idx < MAX_SAVED_FLOORS; fl_idx++) {
        sf_ptr = &saved_floors[fl_idx];
        if (!sf_ptr->floor_id)
            break;
    }

    find_oldest_floor_id(creature_ptr, sf_ptr, &fl_idx);
    sf_ptr->savefile_id = fl_idx;
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
