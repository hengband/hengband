#include "world/world-movement-processor.h"
#include "cmd-io/cmd-save.h"
#include "core/disturbance.h"
#include "dungeon/quest.h"
#include "floor/floor-mode-changer.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "system/angband-system.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-record.h"
#include "system/enums/dungeon/dungeon-id.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "util/enum-range.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief プレイヤーの現在ダンジョンIDと階層に応じて、ダンジョン内ランクエの自動放棄を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void check_random_quest_auto_failure(PlayerType *player_ptr)
{
    auto &quests = QuestList::get_instance();
    const auto &floor = *player_ptr->current_floor_ptr;
    if (floor.dungeon_id != DungeonId::ANGBAND) {
        return;
    }

    auto &world = AngbandWorld::get_instance();
    for (auto quest_id : RANDOM_QUEST_ID_RANGE) {
        auto &quest = quests.get_quest(quest_id);
        auto is_taken_quest = (quest.type == QuestKindType::RANDOM);
        is_taken_quest &= (quest.status == QuestStatusType::TAKEN);
        is_taken_quest &= (quest.level < floor.dun_level);
        if (!is_taken_quest) {
            continue;
        }

        quest.status = QuestStatusType::FAILED;
        quest.complev = (byte)player_ptr->lev;
        world.update_playtime();
        quest.comptime = world.play_time;
        quest.get_bounty().misc_flags.reset(MonsterMiscType::QUESTOR);
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
void execute_recall(PlayerType *player_ptr)
{
    if (player_ptr->word_recall == 0) {
        return;
    }

    if (autosave_l && (player_ptr->word_recall == 1) && !AngbandSystem::get_instance().is_phase_out()) {
        do_cmd_save_game(player_ptr, true);
    }

    player_ptr->word_recall--;
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (player_ptr->word_recall != 0) {
        return;
    }

    disturb(player_ptr, false, true);
    auto &floor = *player_ptr->current_floor_ptr;
    if (floor.is_underground() || floor.is_in_quest() || player_ptr->enter_dungeon) {
        msg_print(_("上に引っ張りあげられる感じがする！", "You feel yourself yanked upwards!"));
        if (floor.is_underground()) {
            player_ptr->recall_dungeon = floor.dungeon_id;
        }
        if (record_stair) {
            exe_write_diary(floor, DiaryKind::RECALL, floor.dun_level);
        }

        floor.dun_level = 0;
        floor.reset_dungeon_index();
        leave_quest_check(player_ptr);
        leave_tower_check(player_ptr);
        floor.quest_number = QuestId::NONE;
        player_ptr->leaving = true;
        sound(SOUND_TPLEVEL);
        return;
    }

    msg_print(_("下に引きずり降ろされる感じがする！", "You feel yourself yanked downwards!"));
    floor.set_dungeon_index(player_ptr->recall_dungeon);
    if (record_stair) {
        exe_write_diary(floor, DiaryKind::RECALL, floor.dun_level);
    }

    floor.dun_level = DungeonRecords::get_instance().get_record(floor.dungeon_id).get_max_level();
    if (!floor.is_underground()) {
        floor.dun_level = 1;
    }
    if (ironman_nightmare && !randint0(666) && (floor.dungeon_id == DungeonId::ANGBAND)) {
        if (floor.dun_level < 50) {
            floor.dun_level *= 2;
        } else if (floor.dun_level < 99) {
            floor.dun_level = (floor.dun_level + 99) / 2;
        } else if (floor.dun_level > 100) {
            floor.dun_level = floor.get_dungeon_definition().maxdepth - 1;
        }
    }

    auto &world = AngbandWorld::get_instance();
    if (world.is_wild_mode()) {
        player_ptr->wilderness_y = player_ptr->y;
        player_ptr->wilderness_x = player_ptr->x;
    } else {
        player_ptr->oldpx = player_ptr->x;
        player_ptr->oldpy = player_ptr->y;
    }

    world.set_wild_mode(false);

    /*
     * Clear all saved floors
     * and create a first saved floor
     */
    FloorChangeModesStore::get_instace()->set(FloorChangeMode::FIRST_FLOOR);
    player_ptr->leaving = true;

    check_random_quest_auto_failure(player_ptr);
    sound(SOUND_TPLEVEL);
}

/*!
 * @brief 10ゲームターンが進行するごとにフロア・リセット/現実変容の残り時間カウントダウンと発動を処理する。
 * / Handle involuntary movement once every 10 game turns
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void execute_floor_reset(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    if (player_ptr->alter_reality == 0) {
        return;
    }

    if (autosave_l && (player_ptr->alter_reality == 1) && !AngbandSystem::get_instance().is_phase_out()) {
        do_cmd_save_game(player_ptr, true);
    }

    player_ptr->alter_reality--;
    RedrawingFlagsUpdater::get_instance().set_flag(MainWindowRedrawingFlag::TIMED_EFFECT);
    if (player_ptr->alter_reality != 0) {
        return;
    }

    disturb(player_ptr, false, true);
    if (!inside_quest(floor.get_quest_id()) && floor.is_underground()) {
        msg_print(_("世界が変わった！", "The world changes!"));
        FloorChangeModesStore::get_instace()->set(FloorChangeMode::FIRST_FLOOR);
        player_ptr->leaving = true;
    } else {
        msg_print(_("世界が少しの間変化したようだ。", "The world seems to change for a moment!"));
    }

    sound(SOUND_TPLEVEL);
}
