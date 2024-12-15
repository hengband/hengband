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
#include "floor/floor-mode-changer.h"
#include "floor/floor-save-util.h"
#include "io/files-util.h"
#include "io/uid-checker.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "term/z-form.h"
#include "util/angband-files.h"
#include "view/display-messages.h"
#include "world/world.h"

static std::string get_saved_floor_name(int level)
{
    const auto ext = format(".F%02d", level);
    return savefile.string().append(ext);
}

static void check_saved_tmp_files(const int fd, bool *force)
{
    if (fd >= 0) {
        (void)fd_close(fd);
        return;
    }

    if (*force) {
        return;
    }

    msg_print(_("エラー：古いテンポラリ・ファイルが残っています。", "Error: There are old temporary files."));
    msg_print(_("変愚蛮怒を二重に起動していないか確認してください。", "Make sure you are not running two game processes simultaneously."));
    msg_print(_("過去に変愚蛮怒がクラッシュした場合は一時ファイルを", "If the temporary files are garbage from an old crashed process, "));
    msg_print(_("強制的に削除して実行を続けられます。", "you can delete them safely."));
    if (!input_check(_("強制的に削除してもよろしいですか？", "Do you delete the old temporary files? "))) {
        quit(_("実行中止", "Aborted."));
    }

    *force = true;
}

/*!
 * @brief 保存フロア配列を初期化する / Initialize saved_floors array.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param force テンポラリファイルが残っていた場合も警告なしで強制的に削除するフラグ
 * @details Make sure that old temporary files are not remaining as gurbages.
 */
void init_saved_floors(bool force)
{
    auto fd = -1;
    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        auto floor_savefile = get_saved_floor_name(i);
        safe_setuid_grab();
        fd = fd_make(floor_savefile);
        safe_setuid_drop();
        check_saved_tmp_files(fd, &force);
        safe_setuid_grab();
        (void)fd_kill(floor_savefile);
        safe_setuid_drop();
        sf_ptr->floor_id = 0;
    }

    max_floor_id = 1;
    latest_visit_mark = 1;
    saved_floor_file_sign = (uint32_t)time(nullptr);
    new_floor_id = 0;
    FloorChangeModesStore::get_instace()->clear();
}

/*!
 * @brief 保存フロア用テンポラリファイルを削除する / Kill temporary files
 * @details Should be called just before the game quit.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void clear_saved_floor_files(PlayerType *player_ptr)
{
    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        if (!is_saved_floor(sf_ptr) || (sf_ptr->floor_id == player_ptr->floor_id)) {
            continue;
        }

        safe_setuid_grab();
        (void)fd_kill(get_saved_floor_name(i));
        safe_setuid_drop();
    }
}

/*!
 * @brief 保存フロアIDから参照ポインタを得る / Get a pointer for an item of the saved_floors array.
 * @param floor_id 保存フロアID
 * @return IDに対応する保存フロアのポインタ、ない場合はnullptrを返す。
 */
saved_floor_type *get_sf_ptr(FLOOR_IDX floor_id)
{
    if (!floor_id) {
        return nullptr;
    }

    for (int i = 0; i < MAX_SAVED_FLOORS; i++) {
        saved_floor_type *sf_ptr = &saved_floors[i];
        if (sf_ptr->floor_id == floor_id) {
            return sf_ptr;
        }
    }

    return nullptr;
}

/*!
 * @brief 参照ポインタ先の保存フロアを抹消する / kill a saved floor and get an empty space
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param sf_ptr 保存フロアの参照ポインタ
 */
void kill_saved_floor(PlayerType *player_ptr, saved_floor_type *sf_ptr)
{
    if (!sf_ptr || !is_saved_floor(sf_ptr)) {
        return;
    }

    if (sf_ptr->floor_id == player_ptr->floor_id) {
        player_ptr->floor_id = 0;
        sf_ptr->floor_id = 0;
        return;
    }

    safe_setuid_grab();
    (void)fd_kill(get_saved_floor_name((int)sf_ptr->savefile_id));
    safe_setuid_drop();
    sf_ptr->floor_id = 0;
}

static FLOOR_IDX find_oldest_floor_idx(PlayerType *player_ptr)
{
    FLOOR_IDX oldest_floor_idx = 0;
    uint32_t oldest_visit = 0xffffffffL;

    for (FLOOR_IDX fl_idx = 0; fl_idx < MAX_SAVED_FLOORS; fl_idx++) {
        const saved_floor_type *sf_ptr = &saved_floors[fl_idx];
        if ((sf_ptr->floor_id == player_ptr->floor_id) || (sf_ptr->visit_mark > oldest_visit)) {
            continue;
        }

        oldest_floor_idx = fl_idx;
        oldest_visit = sf_ptr->visit_mark;
    }

    return oldest_floor_idx;
}

/*!
 * @brief 新規に利用可能なフロアIDを返す / Initialize new floor and get its floor id.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 利用可能なフロアID
 * @details
 * If number of saved floors are already MAX_SAVED_FLOORS, kill the oldest one.
 */
FLOOR_IDX get_unused_floor_id(PlayerType *player_ptr)
{
    saved_floor_type *sf_ptr = nullptr;
    FLOOR_IDX fl_idx;
    for (fl_idx = 0; fl_idx < MAX_SAVED_FLOORS; fl_idx++) {
        sf_ptr = &saved_floors[fl_idx];
        if (!is_saved_floor(sf_ptr)) {
            break;
        }
    }

    if (fl_idx == MAX_SAVED_FLOORS) {
        fl_idx = find_oldest_floor_idx(player_ptr);
        sf_ptr = &saved_floors[fl_idx];
        kill_saved_floor(player_ptr, sf_ptr);
    }

    sf_ptr->savefile_id = fl_idx;
    sf_ptr->floor_id = max_floor_id;
    sf_ptr->last_visit = 0;
    sf_ptr->upper_floor_id = 0;
    sf_ptr->lower_floor_id = 0;
    sf_ptr->visit_mark = latest_visit_mark++;
    sf_ptr->dun_level = player_ptr->current_floor_ptr->dun_level;
    if (max_floor_id < MAX_SHORT) {
        max_floor_id++;
    } else {
        max_floor_id = 1;
    } // 32767 floor_ids are all used up!  Re-use ancient IDs.

    return sf_ptr->floor_id;
}

/*!
 * @brief フロアにいるペットの数を数える
 * @todo party_mon をPartyMonsters クラスに組み上げてそのオブジェクトメソッドに繰り込む
 */
void precalc_cur_num_of_pet()
{
    const auto max_num = AngbandWorld::get_instance().is_wild_mode() ? 1 : MAX_PARTY_MON;
    for (auto i = 0; i < max_num; i++) {
        auto &monster = party_mon[i];
        if (!monster.is_valid()) {
            continue;
        }

        monster.get_real_monrace().increment_current_numbers();
    }
}
