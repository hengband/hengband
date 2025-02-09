#include "target/target-getter.h"
#include "core/asking-player.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "monster/monster-describer.h"
#include "monster/monster-status.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "timed-effect/timed-effects.h"
#include "util/finalizer.h"
#include "view/display-messages.h"
#include <string>

/*
 * Get an "aiming direction" from the user.
 *
 * The "dir" is loaded with 1,2,3,4,6,7,8,9 for "actual direction", and
 * "0" for "current target", and "-1" for "entry aborted".
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Note that confusion over-rides any (explicit?) user choice.
 */
bool get_aim_dir(PlayerType *player_ptr, int *dp)
{
    auto dir = command_dir;
    if (use_old_target && target_okay(player_ptr)) {
        dir = 5;
    }

    short code = 0;
    if (repeat_pull(&code) && Direction::is_valid_dir(code)) {
        if (!(code == 5 && !target_okay(player_ptr))) {
            dir = code;
        }
    }

    *dp = code;
    while (dir == 0) {
        std::string prompt;
        if (!target_okay(player_ptr)) {
            prompt = _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");
        } else {
            prompt = _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ");
        }

        const auto command_opt = input_command(prompt, true);
        if (!command_opt) {
            break;
        }

        auto command = *command_opt;
        if (use_menu && (command == '\r')) {
            command = 't';
        }

        switch (command) {
        case 'T':
        case 't':
        case '.':
        case '5':
        case '0':
            dir = 5;
            break;
        case '*':
        case ' ':
        case '\r':
            if (target_set(player_ptr, TARGET_KILL)) {
                dir = 5;
            }

            break;
        default:
            dir = get_keymap_dir(command);
            break;
        }

        if ((dir == 5) && !target_okay(player_ptr)) {
            dir = 0;
        }

        if (dir == 0) {
            bell();
        }
    }

    if (dir == 0) {
        project_length = 0;
        return false;
    }

    command_dir = dir;
    if (player_ptr->effects()->confusion().is_confused()) {
        dir = rand_choice(Direction::directions_8()).dir();
    }

    if (command_dir != dir) {
        msg_print(_("あなたは混乱している。", "You are confused."));
    }

    *dp = dir;
    repeat_push((COMMAND_CODE)command_dir);
    return true;
}

/*!
 * @brief 方向を指定する(テンキー配列順)
 *
 * 上下左右および斜め方向を指定する。
 * 指定された方向は整数で返され、それぞれの値方向は以下の通り。
 *
 * 7 8 9
 *  \|/
 * 4-@-6
 *  /|\
 * 1 2 3
 *
 * @return 指定した方向。指定をキャンセルした場合はstd::nullopt。
 */
std::optional<int> get_direction(PlayerType *player_ptr)
{
    auto dir = command_dir;
    short code = 0;
    if (repeat_pull(&code) && Direction::is_valid_dir(code)) {
        dir = code;
    }

    constexpr auto prompt = _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
    while (dir == 0) {
        const auto command = input_command(prompt, true);
        if (!command) {
            return std::nullopt;
        }

        dir = get_keymap_dir(*command);
        if (dir == 0) {
            bell();
        }
    }

    command_dir = dir;
    const auto finalizer = util::make_finalizer([] {
        repeat_push(static_cast<short>(command_dir));
    });
    const auto is_confused = player_ptr->effects()->confusion().is_confused();
    if (is_confused && evaluate_percent(75)) {
        dir = rand_choice(Direction::directions_8()).dir();
    }

    if (command_dir == dir) {
        return dir;
    }

    if (is_confused) {
        msg_print(_("あなたは混乱している。", "You are confused."));
        return dir;
    }

    const auto &monster = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
    const auto m_name = monster_desc(player_ptr, monster, 0);
    const auto fmt = monster.is_confused()
                         ? _("%sは混乱している。", "%s^ is confused.")
                         : _("%sは思い通りに動いてくれない。", "You cannot control %s.");
    msg_format(fmt, m_name.data());
    return dir;
}

/*!
 * @brief 方向を指定する(円周順)
 *
 * 上下左右および斜め方向を指定する。
 * 指定された方向は整数で返され、それぞれの値方向は以下の通り。
 *
 * 5 4 3
 *  \|/
 * 6-@-2
 *  /|\
 * 7 0 1
 *
 * @return 指定した方向。指定をキャンセルした場合はstd::nullopt。
 */
std::optional<int> get_direction_as_cdir(PlayerType *player_ptr)
{
    constexpr std::array<int, 10> cdirs = { { 0, 7, 0, 1, 6, 0, 2, 5, 4, 3 } };

    const auto dir = get_direction(player_ptr);
    if (!dir || (dir <= 0) || (dir == 5) || (dir >= std::ssize(cdirs))) {
        return std::nullopt;
    }

    return cdirs[*dir];
}

/*
 * @brief 進行方向を指定する(騎乗対象の混乱の影響を受ける) / Request a "movement" direction (1,2,3,4,6,7,8,9) from the user,
 * and place it into "command_dir", unless we already have one.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc, as well
 * as all commands which must reference a grid adjacent to the player,
 * and which may not reference the grid under the player.  Note that,
 * for example, it is no longer possible to "disarm" or "open" chests
 * in the same grid as the player.
 *
 * Direction "5" is illegal and will (cleanly) abort the command.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", to which "confusion" is applied.
 */
bool get_rep_dir(PlayerType *player_ptr, int *dp, bool under)
{
    auto dir = command_dir;
    short code = 0;
    if (repeat_pull(&code) && Direction::is_valid_dir(code)) {
        dir = code;
    }

    *dp = code;
    const auto prompt = under ? _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ")
                              : _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
    while (dir == 0) {
        const auto command = input_command(prompt, true);
        if (!command) {
            return false;
        }

        if (under && ((command == '5') || (command == '-') || (command == '.'))) {
            dir = 5;
            break;
        }

        dir = get_keymap_dir(*command);
        if (dir == 0) {
            bell();
        }
    }

    if ((dir == 5) && !under) {
        return false;
    }

    command_dir = dir;
    auto is_confused = player_ptr->effects()->confusion().is_confused();
    if (is_confused) {
        if (evaluate_percent(75)) {
            dir = rand_choice(Direction::directions_8()).dir();
        }
    } else if (player_ptr->riding) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        const auto &monrace = monster.get_monrace();
        if (monster.is_confused()) {
            if (evaluate_percent(75)) {
                dir = rand_choice(Direction::directions_8()).dir();
            }
        } else if (monrace.behavior_flags.has_all_of({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 }) && one_in_(2)) {
            dir = rand_choice(Direction::directions_8()).dir();
        } else if (monrace.behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50) && one_in_(4)) {
            dir = rand_choice(Direction::directions_8()).dir();
        }
    }

    if (command_dir != dir) {
        if (is_confused) {
            msg_print(_("あなたは混乱している。", "You are confused."));
        } else {
            const auto &monster = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
            const auto m_name = monster_desc(player_ptr, monster, 0);
            if (monster.is_confused()) {
                msg_format(_("%sは混乱している。", "%s^ is confused."), m_name.data());
            } else {
                msg_format(_("%sは思い通りに動いてくれない。", "You cannot control %s."), m_name.data());
            }
        }
    }

    *dp = dir;
    repeat_push(static_cast<short>(command_dir));
    return true;
}
