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
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
void check_random_quest_auto_failure(player_type *creature_ptr)
{
    if (creature_ptr->dungeon_idx != DUNGEON_ANGBAND) {
        return;
    }
    for (auto i = MIN_RANDOM_QUEST; i < MAX_RANDOM_QUEST + 1; i++) {
        auto q_ptr = &quest[i];
        if ((q_ptr->type == QUEST_TYPE_RANDOM) && (q_ptr->status == QUEST_STATUS_TAKEN) && (q_ptr->level < creature_ptr->current_floor_ptr->dun_level)) {
            q_ptr->status = QUEST_STATUS_FAILED;
            q_ptr->complev = (byte)creature_ptr->lev;
            update_playtime();
            q_ptr->comptime = current_world_ptr->play_time;
            r_info[q_ptr->r_idx].flags1 &= ~(RF1_QUESTOR);
        }
    }
}

/*!
 * @brief 10ゲームターンが進行するごとに帰還の残り時間カウントダウンと発動を処理する。
 * / Handle involuntary movement once every 10 game turns
 * @param creature_ptr プレーヤーへの参照ポインタ
 * @details
 * Autosave BEFORE resetting the recall counter (rr9)
 * The player is yanked up/down as soon as he loads the autosaved game.
 */
void execute_recall(player_type *creature_ptr)
{
    if (creature_ptr->word_recall == 0)
        return;

    if (autosave_l && (creature_ptr->word_recall == 1) && !creature_ptr->phase_out)
        do_cmd_save_game(creature_ptr, true);

    creature_ptr->word_recall--;
    creature_ptr->redraw |= (PR_STATUS);
    if (creature_ptr->word_recall != 0)
        return;

    disturb(creature_ptr, false, true);
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (floor_ptr->dun_level || creature_ptr->current_floor_ptr->inside_quest || creature_ptr->enter_dungeon) {
        msg_print(_("上に引っ張りあげられる感じがする！", "You feel yourself yanked upwards!"));
        if (creature_ptr->dungeon_idx)
            creature_ptr->recall_dungeon = creature_ptr->dungeon_idx;
        if (record_stair)
            exe_write_diary(creature_ptr, DIARY_RECALL, floor_ptr->dun_level, nullptr);

        floor_ptr->dun_level = 0;
        creature_ptr->dungeon_idx = 0;
        leave_quest_check(creature_ptr);
        leave_tower_check(creature_ptr);
        creature_ptr->current_floor_ptr->inside_quest = 0;
        creature_ptr->leaving = true;
        sound(SOUND_TPLEVEL);
        return;
    }

    msg_print(_("下に引きずり降ろされる感じがする！", "You feel yourself yanked downwards!"));
    creature_ptr->dungeon_idx = creature_ptr->recall_dungeon;
    if (record_stair)
        exe_write_diary(creature_ptr, DIARY_RECALL, floor_ptr->dun_level, nullptr);

    floor_ptr->dun_level = max_dlv[creature_ptr->dungeon_idx];
    if (floor_ptr->dun_level < 1)
        floor_ptr->dun_level = 1;
    if (ironman_nightmare && !randint0(666) && (creature_ptr->dungeon_idx == DUNGEON_ANGBAND)) {
        if (floor_ptr->dun_level < 50) {
            floor_ptr->dun_level *= 2;
        } else if (floor_ptr->dun_level < 99) {
            floor_ptr->dun_level = (floor_ptr->dun_level + 99) / 2;
        } else if (floor_ptr->dun_level > 100) {
            floor_ptr->dun_level = d_info[creature_ptr->dungeon_idx].maxdepth - 1;
        }
    }

    if (creature_ptr->wild_mode) {
        creature_ptr->wilderness_y = creature_ptr->y;
        creature_ptr->wilderness_x = creature_ptr->x;
    } else {
        creature_ptr->oldpx = creature_ptr->x;
        creature_ptr->oldpy = creature_ptr->y;
    }

    creature_ptr->wild_mode = false;

    /*
     * Clear all saved floors
     * and create a first saved floor
     */
    prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
    creature_ptr->leaving = true;

    check_random_quest_auto_failure(creature_ptr);
    sound(SOUND_TPLEVEL);
}

/*!
 * @brief 10ゲームターンが進行するごとにフロア・リセット/現実変容の残り時間カウントダウンと発動を処理する。
 * / Handle involuntary movement once every 10 game turns
 * @param creature_ptr プレーヤーへの参照ポインタ
 */
void execute_floor_reset(player_type *creature_ptr)
{
    floor_type *floor_ptr = creature_ptr->current_floor_ptr;
    if (creature_ptr->alter_reality == 0)
        return;

    if (autosave_l && (creature_ptr->alter_reality == 1) && !creature_ptr->phase_out)
        do_cmd_save_game(creature_ptr, true);

    creature_ptr->alter_reality--;
    creature_ptr->redraw |= (PR_STATUS);
    if (creature_ptr->alter_reality != 0)
        return;

    disturb(creature_ptr, false, true);
    if (!quest_number(creature_ptr, floor_ptr->dun_level) && floor_ptr->dun_level) {
        msg_print(_("世界が変わった！", "The world changes!"));

        /*
         * Clear all saved floors
         * and create a first saved floor
         */
        prepare_change_floor_mode(creature_ptr, CFM_FIRST_FLOOR);
        creature_ptr->leaving = true;
    } else {
        msg_print(_("世界が少しの間変化したようだ。", "The world seems to change for a moment!"));
    }

    sound(SOUND_TPLEVEL);
}
