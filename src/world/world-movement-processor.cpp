#include "world/world-movement-processor.h"
#include "cmd-io/cmd-save.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/floor-mode-changer.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "system/floor-type-definition.h"
#include "system/monster-race-definition.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"


/*!
 * @brief プレイヤーの現在ダンジョンIDと階層に応じて、ダンジョン内ランクエの自動放棄を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void check_random_quest_auto_failure(player_type *player_ptr)
{
    if (player_ptr->dungeon_idx != DUNGEON_ANGBAND) {
        return;
    }
    for (auto i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++) {
        auto q_ptr = &quest[i];
        if ((q_ptr->type == QuestKindType::RANDOM) && (q_ptr->status == QuestStatusType::TAKEN) && (q_ptr->level < player_ptr->current_floor_ptr->dun_level)) {
            q_ptr->status = QuestStatusType::FAILED;
            q_ptr->complev = (byte)player_ptr->lev;
            update_playtime();
            q_ptr->comptime = w_ptr->play_time;
            r_info[q_ptr->r_idx].flags1 &= ~(RF1_QUESTOR);
        }
    }
}

/*!
 * @brief 10ゲームターンが進行するごとに帰還の残り時間カウントダウンと発動を処理する。
 * / Handle involuntary movement once every 10 game turns
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * Autosave BEFORE resetting the recall counter (rr9)
 * The player is yanked up/down as soon as he loads the autosaved game.
 */
void execute_recall(player_type *player_ptr)
{
    if (player_ptr->word_recall == 0)
        return;

    if (autosave_l && (player_ptr->word_recall == 1) && !player_ptr->phase_out)
        do_cmd_save_game(player_ptr, true);

    player_ptr->word_recall--;
    player_ptr->redraw |= (PR_STATUS);
    if (player_ptr->word_recall != 0)
        return;

    disturb(player_ptr, false, true);
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (floor_ptr->dun_level || player_ptr->current_floor_ptr->inside_quest || player_ptr->enter_dungeon) {
        msg_print(_("上に引っ張りあげられる感じがする！", "You feel yourself yanked upwards!"));
        if (player_ptr->dungeon_idx)
            player_ptr->recall_dungeon = player_ptr->dungeon_idx;
        if (record_stair)
            exe_write_diary(player_ptr, DIARY_RECALL, floor_ptr->dun_level, nullptr);

        floor_ptr->dun_level = 0;
        player_ptr->dungeon_idx = 0;
        leave_quest_check(player_ptr);
        leave_tower_check(player_ptr);
        player_ptr->current_floor_ptr->inside_quest = 0;
        player_ptr->leaving = true;
        sound(SOUND_TPLEVEL);
        return;
    }

    msg_print(_("下に引きずり降ろされる感じがする！", "You feel yourself yanked downwards!"));
    player_ptr->dungeon_idx = player_ptr->recall_dungeon;
    if (record_stair)
        exe_write_diary(player_ptr, DIARY_RECALL, floor_ptr->dun_level, nullptr);

    floor_ptr->dun_level = max_dlv[player_ptr->dungeon_idx];
    if (floor_ptr->dun_level < 1)
        floor_ptr->dun_level = 1;
    if (ironman_nightmare && !randint0(666) && (player_ptr->dungeon_idx == DUNGEON_ANGBAND)) {
        if (floor_ptr->dun_level < 50) {
            floor_ptr->dun_level *= 2;
        } else if (floor_ptr->dun_level < 99) {
            floor_ptr->dun_level = (floor_ptr->dun_level + 99) / 2;
        } else if (floor_ptr->dun_level > 100) {
            floor_ptr->dun_level = d_info[player_ptr->dungeon_idx].maxdepth - 1;
        }
    }

    if (player_ptr->wild_mode) {
        player_ptr->wilderness_y = player_ptr->y;
        player_ptr->wilderness_x = player_ptr->x;
    } else {
        player_ptr->oldpx = player_ptr->x;
        player_ptr->oldpy = player_ptr->y;
    }

    player_ptr->wild_mode = false;

    /*
     * Clear all saved floors
     * and create a first saved floor
     */
    prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR);
    player_ptr->leaving = true;

    check_random_quest_auto_failure(player_ptr);
    sound(SOUND_TPLEVEL);
}

/*!
 * @brief 10ゲームターンが進行するごとにフロア・リセット/現実変容の残り時間カウントダウンと発動を処理する。
 * / Handle involuntary movement once every 10 game turns
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void execute_floor_reset(player_type *player_ptr)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    if (player_ptr->alter_reality == 0)
        return;

    if (autosave_l && (player_ptr->alter_reality == 1) && !player_ptr->phase_out)
        do_cmd_save_game(player_ptr, true);

    player_ptr->alter_reality--;
    player_ptr->redraw |= (PR_STATUS);
    if (player_ptr->alter_reality != 0)
        return;

    disturb(player_ptr, false, true);
    if (!quest_number(player_ptr, floor_ptr->dun_level) && floor_ptr->dun_level) {
        msg_print(_("世界が変わった！", "The world changes!"));

        /*
         * Clear all saved floors
         * and create a first saved floor
         */
        prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR);
        player_ptr->leaving = true;
    } else {
        msg_print(_("世界が少しの間変化したようだ。", "The world seems to change for a moment!"));
    }

    sound(SOUND_TPLEVEL);
}
