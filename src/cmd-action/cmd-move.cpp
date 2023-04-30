#include "cmd-action/cmd-move.h"
#include "action/action-limited.h"
#include "action/movement-execution.h"
#include "action/run-execution.h"
#include "avatar/avatar.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "core/disturbance.h"
#include "core/player-redraw-types.h"
#include "core/player-update-types.h"
#include "core/stuff-handler.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-mode-changer.h"
#include "floor/wild.h"
#include "game-option/birth-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "info-reader/fixed-map-parser.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "mind/mind-ninja.h"
#include "player-base/player-class.h"
#include "player-info/samurai-data-type.h"
#include "player-status/player-energy.h"
#include "player/attack-defense-types.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "player/special-defense-types.h"
#include "spell-realm/spells-hex.h"
#include "spell-realm/spells-song.h"
#include "status/action-setter.h"
#include "system/dungeon-info.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain-type-definition.h"
#include "target/target-getter.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief フロア脱出時に出戻りが不可能だった場合に警告を加える処理
 * @param down_stair TRUEならば階段を降りる処理、FALSEなら階段を昇る処理による内容
 * @return フロア移動を実際に行うならTRUE、キャンセルする場合はFALSE
 */
static bool confirm_leave_level(PlayerType *player_ptr, bool down_stair)
{
    const auto &quest_list = QuestList::get_instance();
    const auto *q_ptr = &quest_list[player_ptr->current_floor_ptr->quest_number];

    auto caution_in_tower = any_bits(q_ptr->flags, QUEST_FLAG_TOWER);
    caution_in_tower &= q_ptr->status != QuestStatusType::STAGE_COMPLETED || (down_stair && (quest_list[QuestId::TOWER1].status != QuestStatusType::COMPLETED));

    auto caution_in_quest = q_ptr->type == QuestKindType::RANDOM;
    caution_in_quest |= q_ptr->flags & QUEST_FLAG_ONCE && q_ptr->status != QuestStatusType::COMPLETED;
    caution_in_quest |= caution_in_tower;

    if (confirm_quest && inside_quest(player_ptr->current_floor_ptr->quest_number) && caution_in_quest) {
        msg_print(_("この階を一度去ると二度と戻って来られません。", "You can't come back here once you leave this floor."));
        return get_check(_("本当にこの階を去りますか？", "Really leave this floor? "));
    }

    return true;
}

/*!
 * @brief 階段を使って階層を昇る処理 / Go up one level
 */
void do_cmd_go_up(PlayerType *player_ptr)
{
    auto &quest_list = QuestList::get_instance();
    bool go_up = false;
    auto *g_ptr = &player_ptr->current_floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    auto *f_ptr = &terrains_info[g_ptr->feat];
    int up_num = 0;
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (f_ptr->flags.has_not(TerrainCharacteristics::LESS)) {
        msg_print(_("ここには上り階段が見当たらない。", "I see no up staircase here."));
        return;
    }

    const auto &floor_ptr = player_ptr->current_floor_ptr;
    if (f_ptr->flags.has(TerrainCharacteristics::QUEST)) {
        if (!confirm_leave_level(player_ptr, false)) {
            return;
        }

        if (is_echizen(player_ptr)) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("上の階に登った。", "You enter the up staircase."));
        }

        sound(SOUND_STAIRWAY);

        leave_quest_check(player_ptr);
        floor_ptr->quest_number = i2enum<QuestId>(g_ptr->special);
        const auto quest_number = floor_ptr->quest_number;
        auto &quest = quest_list[quest_number];
        if (quest.status == QuestStatusType::UNTAKEN) {
            if (quest.type != QuestKindType::RANDOM) {
                init_flags = INIT_ASSIGN;
                parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
            }

            quest.status = QuestStatusType::TAKEN;
        }

        if (!inside_quest(quest_number)) {
            floor_ptr->dun_level = 0;
            player_ptr->word_recall = 0;
        }

        player_ptr->leaving = true;
        player_ptr->oldpx = 0;
        player_ptr->oldpy = 0;
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        return;
    }

    if (!floor_ptr->is_in_dungeon()) {
        go_up = true;
    } else {
        go_up = confirm_leave_level(player_ptr, false);
    }

    if (!go_up) {
        return;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);

    if (autosave_l) {
        do_cmd_save_game(player_ptr, true);
    }

    const auto quest_number = player_ptr->current_floor_ptr->quest_number;
    auto &quest = quest_list[quest_number];

    if (inside_quest(quest_number) && quest.type == QuestKindType::RANDOM) {
        leave_quest_check(player_ptr);
        player_ptr->current_floor_ptr->quest_number = QuestId::NONE;
    }

    if (inside_quest(quest_number) && quest.type != QuestKindType::RANDOM) {
        leave_quest_check(player_ptr);
        player_ptr->current_floor_ptr->quest_number = i2enum<QuestId>(g_ptr->special);
        player_ptr->current_floor_ptr->dun_level = 0;
        up_num = 0;
    } else {
        if (f_ptr->flags.has(TerrainCharacteristics::SHAFT)) {
            prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_UP | CFM_SHAFT);
            up_num = 2;
        } else {
            prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_UP);
            up_num = 1;
        }

        if (player_ptr->current_floor_ptr->dun_level - up_num < dungeons_info[player_ptr->dungeon_idx].mindepth) {
            up_num = player_ptr->current_floor_ptr->dun_level;
        }
    }

    if (record_stair) {
        exe_write_diary(player_ptr, DIARY_STAIR, 0 - up_num, _("階段を上った", "climbed up the stairs to"));
    }

    if (up_num == player_ptr->current_floor_ptr->dun_level) {
        if (is_echizen(player_ptr)) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("地上に戻った。", "You go back to the surface."));
        }
        player_ptr->word_recall = 0;
    } else {
        if (is_echizen(player_ptr)) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("階段を上って新たなる迷宮へと足を踏み入れた。", "You enter a maze of up staircases."));
        }
    }

    sound(SOUND_STAIRWAY);

    player_ptr->leaving = true;
}

/*!
 * @brief 階段を使って階層を降りる処理 / Go down one level
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_go_down(PlayerType *player_ptr)
{
    bool fall_trap = false;
    int down_num = 0;
    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ptr->grid_array[player_ptr->y][player_ptr->x];
    auto *f_ptr = &terrains_info[g_ptr->feat];
    if (f_ptr->flags.has_not(TerrainCharacteristics::MORE)) {
        msg_print(_("ここには下り階段が見当たらない。", "I see no down staircase here."));
        return;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::TRAP)) {
        fall_trap = true;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::QUEST_ENTER)) {
        do_cmd_quest(player_ptr);
        return;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::QUEST)) {
        auto &quest_list = QuestList::get_instance();
        if (!confirm_leave_level(player_ptr, true)) {
            return;
        }

        if (is_echizen(player_ptr)) {
            msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
        } else {
            msg_print(_("下の階に降りた。", "You enter the down staircase."));
        }

        sound(SOUND_STAIRWAY);

        leave_quest_check(player_ptr);
        leave_tower_check(player_ptr);
        floor_ptr->quest_number = i2enum<QuestId>(g_ptr->special);

        auto &current_quest = quest_list[floor_ptr->quest_number];
        if (current_quest.status == QuestStatusType::UNTAKEN) {
            if (current_quest.type != QuestKindType::RANDOM) {
                init_flags = INIT_ASSIGN;
                parse_fixed_map(player_ptr, QUEST_DEFINITION_LIST, 0, 0, 0, 0);
            }

            current_quest.status = QuestStatusType::TAKEN;
        }

        if (!inside_quest(floor_ptr->quest_number)) {
            floor_ptr->dun_level = 0;
            player_ptr->word_recall = 0;
        }

        player_ptr->leaving = true;
        player_ptr->oldpx = 0;
        player_ptr->oldpy = 0;
        PlayerEnergy(player_ptr).set_player_turn_energy(100);
        return;
    }

    DUNGEON_IDX target_dungeon = 0;
    if (!floor_ptr->is_in_dungeon()) {
        target_dungeon = f_ptr->flags.has(TerrainCharacteristics::ENTRANCE) ? g_ptr->special : DUNGEON_ANGBAND;
        if (ironman_downward && (target_dungeon != DUNGEON_ANGBAND)) {
            msg_print(_("ダンジョンの入口は塞がれている！", "The entrance of this dungeon is closed!"));
            return;
        }

        if (!max_dlv[target_dungeon]) {
            const auto mes = _("ここには%sの入り口(%d階相当)があります", "There is the entrance of %s (Danger level: %d)");
            const auto &dungeon = dungeons_info[target_dungeon];
            msg_format(mes, dungeon.name.data(), dungeon.mindepth);
            if (!get_check(_("本当にこのダンジョンに入りますか？", "Do you really get in this dungeon? "))) {
                return;
            }
        }

        player_ptr->oldpx = player_ptr->x;
        player_ptr->oldpy = player_ptr->y;
        player_ptr->dungeon_idx = target_dungeon;
        prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR);
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (autosave_l) {
        do_cmd_save_game(player_ptr, true);
    }

    if (f_ptr->flags.has(TerrainCharacteristics::SHAFT)) {
        down_num += 2;
    } else {
        down_num += 1;
    }

    if (!floor_ptr->is_in_dungeon()) {
        player_ptr->enter_dungeon = true;
        down_num = dungeons_info[player_ptr->dungeon_idx].mindepth;
    }

    if (record_stair) {
        if (fall_trap) {
            exe_write_diary(player_ptr, DIARY_STAIR, down_num, _("落とし戸に落ちた", "fell through a trap door"));
        } else {
            exe_write_diary(player_ptr, DIARY_STAIR, down_num, _("階段を下りた", "climbed down the stairs to"));
        }
    }

    if (fall_trap) {
        msg_print(_("わざと落とし戸に落ちた。", "You deliberately jump through the trap door."));
    } else {
        if (target_dungeon) {
            msg_format(_("%sへ入った。", "You entered %s."), dungeons_info[player_ptr->dungeon_idx].text.data());
        } else {
            if (is_echizen(player_ptr)) {
                msg_print(_("なんだこの階段は！", "What's this STAIRWAY!"));
            } else {
                msg_print(_("階段を下りて新たなる迷宮へと足を踏み入れた。", "You enter a maze of down staircases."));
            }
        }

        sound(SOUND_STAIRWAY);
    }

    player_ptr->leaving = true;

    if (fall_trap) {
        prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_RAND_PLACE | CFM_RAND_CONNECT);
        return;
    }

    if (f_ptr->flags.has(TerrainCharacteristics::SHAFT)) {
        prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_DOWN | CFM_SHAFT);
    } else {
        prepare_change_floor_mode(player_ptr, CFM_SAVE_FLOORS | CFM_DOWN);
    }
}

/*!
 * @brief 「歩く」動作コマンドのメインルーチン /
 * Support code for the "Walk" and "Jump" commands
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pickup アイテムの自動拾いを行うならTRUE
 */
void do_cmd_walk(PlayerType *player_ptr, bool pickup)
{
    if (command_arg) {
        command_rep = command_arg - 1;
        player_ptr->redraw |= PR_STATE;
        command_arg = 0;
    }

    bool more = false;
    DIRECTION dir;
    if (get_rep_dir(player_ptr, &dir, false)) {
        PlayerEnergy energy(player_ptr);
        energy.set_player_turn_energy(100);
        if (dir != 5) {
            PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });
        }

        if (player_ptr->wild_mode) {
            energy.mul_player_turn_energy((MAX_HGT + MAX_WID) / 2);
        }

        if (player_ptr->action == ACTION_HAYAGAKE) {
            auto energy_use = (ENERGY)(player_ptr->energy_use * (45 - (player_ptr->lev / 2)) / 100);
            energy.set_player_turn_energy(energy_use);
        }

        exe_movement(player_ptr, dir, pickup, false);
        more = true;
    }

    if (player_ptr->wild_mode && !cave_has_flag_bold(player_ptr->current_floor_ptr, player_ptr->y, player_ptr->x, TerrainCharacteristics::TOWN)) {
        int tmp = 120 + player_ptr->lev * 10 - wilderness[player_ptr->y][player_ptr->x].level + 5;
        if (tmp < 1) {
            tmp = 1;
        }

        if (((wilderness[player_ptr->y][player_ptr->x].level + 5) > (player_ptr->lev / 2)) && randint0(tmp) < (21 - player_ptr->skill_stl)) {
            msg_print(_("襲撃だ！", "You are ambushed !"));
            player_ptr->oldpy = randint1(MAX_HGT - 2);
            player_ptr->oldpx = randint1(MAX_WID - 2);
            change_wild_mode(player_ptr, true);
            PlayerEnergy(player_ptr).set_player_turn_energy(100);
        }
    }

    if (!more) {
        disturb(player_ptr, false, false);
    }
}

/*!
 * @brief 「走る」動作コマンドのメインルーチン /
 * Start running.
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_run(PlayerType *player_ptr)
{
    DIRECTION dir;
    if (cmd_limit_confused(player_ptr)) {
        return;
    }

    PlayerClass(player_ptr).break_samurai_stance({ SamuraiStanceType::MUSOU });

    if (get_rep_dir(player_ptr, &dir, false)) {
        player_ptr->running = (command_arg ? command_arg : 1000);
        run_step(player_ptr, dir);
    }
}

/*!
 * @brief 「留まる」動作コマンドのメインルーチン /
 * Stay still.  Search.  Enter stores.
 * Pick up treasure if "pickup" is true.
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pickup アイテムの自動拾いを行うならTRUE
 */
void do_cmd_stay(PlayerType *player_ptr, bool pickup)
{
    uint32_t mpe_mode = MPE_STAYING | MPE_ENERGY_USE;
    if (command_arg) {
        command_rep = command_arg - 1;
        player_ptr->redraw |= (PR_STATE);
        command_arg = 0;
    }

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (pickup) {
        mpe_mode |= MPE_DO_PICKUP;
    }

    (void)move_player_effect(player_ptr, player_ptr->y, player_ptr->x, mpe_mode);
}

/*!
 * @brief 「休む」動作コマンドのメインルーチン /
 * Resting allows a player to safely restore his hp	-RAK-
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_rest(PlayerType *player_ptr)
{
    set_action(player_ptr, ACTION_NONE);
    if (PlayerClass(player_ptr).equals(PlayerClassType::BARD) && ((get_singing_song_effect(player_ptr) != 0) || (get_interrupting_song_effect(player_ptr) != 0))) {
        stop_singing(player_ptr);
    }

    SpellHex spell_hex(player_ptr);
    if (spell_hex.is_spelling_any()) {
        (void)spell_hex.stop_all_spells();
    }

    if (command_arg <= 0) {
        concptr p = _("休憩 (0-9999, '*' で HP/MP全快, '&' で必要なだけ): ", "Rest (0-9999, '*' for HP/SP, '&' as needed): ");
        char out_val[80];
        strcpy(out_val, "&");
        if (!get_string(p, out_val, 4)) {
            return;
        }

        if (out_val[0] == '&') {
            command_arg = COMMAND_ARG_REST_UNTIL_DONE;
        } else if (out_val[0] == '*') {
            command_arg = COMMAND_ARG_REST_FULL_HEALING;
        } else {
            command_arg = (COMMAND_ARG)atoi(out_val);
            if (command_arg <= 0) {
                return;
            }
        }
    }

    if (command_arg > 9999) {
        command_arg = 9999;
    }

    set_superstealth(player_ptr, false);

    PlayerEnergy(player_ptr).set_player_turn_energy(100);
    if (command_arg > 100) {
        chg_virtue(player_ptr, Virtue::DILIGENCE, -1);
    }

    if (player_ptr->is_fully_healthy()) {
        chg_virtue(player_ptr, Virtue::DILIGENCE, -1);
    }

    player_ptr->resting = command_arg;
    player_ptr->action = ACTION_REST;
    player_ptr->update |= PU_BONUS;
    player_ptr->redraw |= (PR_STATE);
    handle_stuff(player_ptr);
    term_fresh();
}
