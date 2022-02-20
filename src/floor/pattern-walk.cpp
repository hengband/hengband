#include "floor/pattern-walk.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "dungeon/dungeon.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/floor-mode-changer.h"
#include "game-option/birth-options.h"
#include "game-option/play-record-options.h"
#include "game-option/special-options.h"
#include "grid/feature.h"
#include "io/input-key-requester.h"
#include "io/write-diary.h"
#include "player-base/player-race.h"
#include "player-status/player-energy.h"
#include "player/player-damage.h"
#include "player/player-move.h"
#include "player/player-status.h"
#include "spell-kind/spells-teleport.h"
#include "spell/spells-status.h"
#include "status/bad-status-setter.h"
#include "status/experience.h"
#include "system/floor-type-definition.h"
#include "system/grid-type-definition.h"
#include "system/player-type-definition.h"
#include "timed-effect/player-cut.h"
#include "timed-effect/player-stun.h"
#include "timed-effect/timed-effects.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "world/world-movement-processor.h"
#include "world/world.h"

/*!
 * @brief パターン終点到達時のテレポート処理を行う
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void pattern_teleport(PlayerType *player_ptr)
{
    DEPTH min_level = 0;
    DEPTH max_level = 99;

    if (get_check(_("他の階にテレポートしますか？", "Teleport level? "))) {
        char ppp[80];
        char tmp_val[160];

        if (ironman_downward)
            min_level = player_ptr->current_floor_ptr->dun_level;

        if (player_ptr->dungeon_idx == DUNGEON_ANGBAND) {
            if (player_ptr->current_floor_ptr->dun_level > 100)
                max_level = MAX_DEPTH - 1;
            else if (player_ptr->current_floor_ptr->dun_level == 100)
                max_level = 100;
        } else {
            max_level = d_info[player_ptr->dungeon_idx].maxdepth;
            min_level = d_info[player_ptr->dungeon_idx].mindepth;
        }

        sprintf(ppp, _("テレポート先:(%d-%d)", "Teleport to level (%d-%d): "), (int)min_level, (int)max_level);
        sprintf(tmp_val, "%d", (int)player_ptr->current_floor_ptr->dun_level);
        if (!get_string(ppp, tmp_val, 10))
            return;

        command_arg = (COMMAND_ARG)atoi(tmp_val);
    } else if (get_check(_("通常テレポート？", "Normal teleport? "))) {
        teleport_player(player_ptr, 200, TELEPORT_SPONTANEOUS);
        return;
    } else {
        return;
    }

    if (command_arg < min_level)
        command_arg = (COMMAND_ARG)min_level;
    if (command_arg > max_level)
        command_arg = (COMMAND_ARG)max_level;

    msg_format(_("%d 階にテレポートしました。", "You teleport to dungeon level %d."), command_arg);
    if (autosave_l)
        do_cmd_save_game(player_ptr, true);

    player_ptr->current_floor_ptr->dun_level = command_arg;
    leave_quest_check(player_ptr);
    if (record_stair)
        exe_write_diary(player_ptr, DIARY_PAT_TELE, 0, nullptr);

    player_ptr->current_floor_ptr->quest_number = QuestId::NONE;
    PlayerEnergy(player_ptr).reset_player_turn();

    /*
     * Clear all saved floors
     * and create a first saved floor
     */
    prepare_change_floor_mode(player_ptr, CFM_FIRST_FLOOR);

    check_random_quest_auto_failure(player_ptr);

    player_ptr->leaving = true;
}

/*!
 * @brief 各種パターン地形上の特別な処理 / Returns TRUE if we are on the Pattern...
 * @return 実際にパターン地形上にプレイヤーが居た場合はTRUEを返す。
 */
bool pattern_effect(PlayerType *player_ptr)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    if (!pattern_tile(floor_ptr, player_ptr->y, player_ptr->x))
        return false;

    auto is_cut = player_ptr->effects()->cut()->is_cut();
    if ((PlayerRace(player_ptr).equals(PlayerRaceType::AMBERITE)) && is_cut && one_in_(10)) {
        wreck_the_pattern(player_ptr);
    }

    int pattern_type = f_info[floor_ptr->grid_array[player_ptr->y][player_ptr->x].feat].subtype;
    switch (pattern_type) {
    case PATTERN_TILE_END:
        (void)BadStatusSetter(player_ptr).hallucination(0);
        (void)restore_all_status(player_ptr);
        (void)restore_level(player_ptr);
        (void)cure_critical_wounds(player_ptr, 1000);

        cave_set_feat(player_ptr, player_ptr->y, player_ptr->x, feat_pattern_old);
        msg_print(_("「パターン」のこの部分は他の部分より強力でないようだ。", "This section of the Pattern looks less powerful."));

        /*
         * We could make the healing effect of the
         * Pattern center one-time only to avoid various kinds
         * of abuse, like luring the win monster into fighting you
         * in the middle of the pattern...
         */
        break;

    case PATTERN_TILE_OLD:
        /* No effect */
        break;

    case PATTERN_TILE_TELEPORT:
        pattern_teleport(player_ptr);
        break;

    case PATTERN_TILE_WRECKED:
        if (!is_invuln(player_ptr))
            take_hit(player_ptr, DAMAGE_NOESCAPE, 200, _("壊れた「パターン」を歩いたダメージ", "walking the corrupted Pattern"));
        break;

    default:
        if (PlayerRace(player_ptr).equals(PlayerRaceType::AMBERITE) && !one_in_(2))
            return true;
        else if (!is_invuln(player_ptr))
            take_hit(player_ptr, DAMAGE_NOESCAPE, damroll(1, 3), _("「パターン」を歩いたダメージ", "walking the Pattern"));
        break;
    }

    return true;
}

/*!
 * @brief パターンによる移動制限処理
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param c_y プレイヤーの移動元Y座標
 * @param c_x プレイヤーの移動元X座標
 * @param n_y プレイヤーの移動先Y座標
 * @param n_x プレイヤーの移動先X座標
 * @return 移動処理が可能である場合（可能な場合に選択した場合）TRUEを返す。
 */
bool pattern_seq(PlayerType *player_ptr, POSITION c_y, POSITION c_x, POSITION n_y, POSITION n_x)
{
    feature_type *cur_f_ptr = &f_info[player_ptr->current_floor_ptr->grid_array[c_y][c_x].feat];
    feature_type *new_f_ptr = &f_info[player_ptr->current_floor_ptr->grid_array[n_y][n_x].feat];
    bool is_pattern_tile_cur = cur_f_ptr->flags.has(FloorFeatureType::PATTERN);
    bool is_pattern_tile_new = new_f_ptr->flags.has(FloorFeatureType::PATTERN);
    if (!is_pattern_tile_cur && !is_pattern_tile_new)
        return true;

    int pattern_type_cur = is_pattern_tile_cur ? cur_f_ptr->subtype : NOT_PATTERN_TILE;
    int pattern_type_new = is_pattern_tile_new ? new_f_ptr->subtype : NOT_PATTERN_TILE;
    if (pattern_type_new == PATTERN_TILE_START) {
        auto effects = player_ptr->effects();
        auto is_stunned = effects->stun()->is_stunned();
        if (!is_pattern_tile_cur && !player_ptr->confused && !is_stunned && !player_ptr->hallucinated) {
            if (get_check(_("パターンの上を歩き始めると、全てを歩かなければなりません。いいですか？",
                    "If you start walking the Pattern, you must walk the whole way. Ok? ")))
                return true;
            else
                return false;
        } else
            return true;
    }

    if ((pattern_type_new == PATTERN_TILE_OLD) || (pattern_type_new == PATTERN_TILE_END) || (pattern_type_new == PATTERN_TILE_WRECKED)) {
        if (is_pattern_tile_cur) {
            return true;
        } else {
            msg_print(_("パターンの上を歩くにはスタート地点から歩き始めなくてはなりません。", "You must start walking the Pattern from the startpoint."));
            return false;
        }
    }

    if ((pattern_type_new == PATTERN_TILE_TELEPORT) || (pattern_type_cur == PATTERN_TILE_TELEPORT))
        return true;

    if (pattern_type_cur == PATTERN_TILE_START) {
        if (is_pattern_tile_new)
            return true;
        else {
            msg_print(_("パターンの上は正しい順序で歩かねばなりません。", "You must walk the Pattern in correct order."));
            return false;
        }
    }

    if ((pattern_type_cur == PATTERN_TILE_OLD) || (pattern_type_cur == PATTERN_TILE_END) || (pattern_type_cur == PATTERN_TILE_WRECKED)) {
        if (!is_pattern_tile_new) {
            msg_print(_("パターンを踏み外してはいけません。", "You may not step off from the Pattern."));
            return false;
        } else {
            return true;
        }
    }

    if (!is_pattern_tile_cur) {
        msg_print(_("パターンの上を歩くにはスタート地点から歩き始めなくてはなりません。", "You must start walking the Pattern from the startpoint."));

        return false;
    }

    byte ok_move = PATTERN_TILE_START;
    switch (pattern_type_cur) {
    case PATTERN_TILE_1:
        ok_move = PATTERN_TILE_2;
        break;
    case PATTERN_TILE_2:
        ok_move = PATTERN_TILE_3;
        break;
    case PATTERN_TILE_3:
        ok_move = PATTERN_TILE_4;
        break;
    case PATTERN_TILE_4:
        ok_move = PATTERN_TILE_1;
        break;
    default:
        if (w_ptr->wizard)
            msg_format(_("おかしなパターン歩行、%d。", "Funny Pattern walking, %d."), pattern_type_cur);
        return true;
    }

    if ((pattern_type_new == ok_move) || (pattern_type_new == pattern_type_cur))
        return true;

    if (!is_pattern_tile_new)
        msg_print(_("パターンを踏み外してはいけません。", "You may not step off from the Pattern."));
    else
        msg_print(_("パターンの上は正しい順序で歩かねばなりません。", "You must walk the Pattern in correct order."));

    return false;
}
