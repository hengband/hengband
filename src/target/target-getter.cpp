#include "target/target-getter.h"
#include "core/asking-player.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/input-options.h"
#include "io/command-repeater.h"
#include "io/input-key-requester.h"
#include "main/sound-of-music.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster/monster-describer.h"
#include "monster/monster-status.h"
#include "system/floor-type-definition.h"
#include "system/monster-entity.h"
#include "system/monster-race-info.h"
#include "system/player-type-definition.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "timed-effect/player-confusion.h"
#include "timed-effect/timed-effects.h"
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

    short code;
    if (repeat_pull(&code)) {
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
        if (!command_opt.has_value()) {
            break;
        }

        auto command = command_opt.value();
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
    if (player_ptr->effects()->confusion()->is_confused()) {
        dir = ddd[randint0(8)];
    }

    if (command_dir != dir) {
        msg_print(_("あなたは混乱している。", "You are confused."));
    }

    *dp = dir;
    repeat_push((COMMAND_CODE)command_dir);
    return true;
}

bool get_direction(PlayerType *player_ptr, int *dp)
{
    auto dir = command_dir;
    short code;
    if (repeat_pull(&code)) {
        dir = code;
    }

    *dp = code;
    constexpr auto prompt = _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
    while (dir == 0) {
        const auto command = input_command(prompt, true);
        if (!command.has_value()) {
            return false;
        }

        const auto ch = command.value();
        dir = get_keymap_dir(ch);
        if (dir == 0) {
            bell();
        }
    }

    command_dir = dir;
    auto is_confused = player_ptr->effects()->confusion()->is_confused();
    if (is_confused && (randint0(100) < 75)) {
        dir = ddd[randint0(8)];
    }

    if (command_dir != dir) {
        if (is_confused) {
            msg_print(_("あなたは混乱している。", "You are confused."));
        } else {
            auto *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            if (m_ptr->is_confused()) {
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
    short code;
    if (repeat_pull(&code)) {
        dir = code;
    }

    *dp = code;
    const auto prompt = under ? _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ")
                              : _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
    while (dir == 0) {
        const auto command = input_command(prompt, true);
        if (!command.has_value()) {
            return false;
        }

        const auto ch = command.value();
        if (under && ((ch == '5') || (ch == '-') || (ch == '.'))) {
            dir = 5;
            break;
        }

        dir = get_keymap_dir(ch);
        if (dir == 0) {
            bell();
        }
    }

    if ((dir == 5) && !under) {
        return false;
    }

    command_dir = dir;
    auto is_confused = player_ptr->effects()->confusion()->is_confused();
    if (is_confused) {
        if (randint0(100) < 75) {
            dir = ddd[randint0(8)];
        }
    } else if (player_ptr->riding) {
        auto *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        auto *r_ptr = &m_ptr->get_monrace();
        if (m_ptr->is_confused()) {
            if (randint0(100) < 75) {
                dir = ddd[randint0(8)];
            }
        } else if (r_ptr->behavior_flags.has_all_of({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 }) && (randint0(100) < 50)) {
            dir = ddd[randint0(8)];
        } else if (r_ptr->behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50) && (randint0(100) < 25)) {
            dir = ddd[randint0(8)];
        }
    }

    if (command_dir != dir) {
        if (is_confused) {
            msg_print(_("あなたは混乱している。", "You are confused."));
        } else {
            auto *m_ptr = &player_ptr->current_floor_ptr->m_list[player_ptr->riding];
            const auto m_name = monster_desc(player_ptr, m_ptr, 0);
            if (m_ptr->is_confused()) {
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
