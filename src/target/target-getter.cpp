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
#include <string_view>
#include <unordered_set>

/*!
 * @brief 上下左右および斜め方向、またはターゲットを指定する
 * @param enable_repeat
 * true(デフォルト)の場合、前回と同じターゲットの自動指定ができ、繰り返しコマンドの対象となる
 * falseの場合、かならず方向かターゲットを選択し、繰り返しコマンドの対象とならない。
 * @return 指定した方向、またはターゲット(座標はグローバル変数target_row/target_colに格納される)
 */
Direction get_aim_dir(PlayerType *player_ptr, bool enable_repeat)
{
    auto dir = Direction::none();
    if (enable_repeat) {
        dir = command_dir;
        auto try_old_target = use_old_target;

        short code = 0;
        if (repeat_pull(&code) && Direction::is_valid_dir(code)) {
            if (code == 5) {
                try_old_target = true;
            } else {
                try_old_target = false;
                dir = Direction(code);
            }
        }

        if (try_old_target && target_okay(player_ptr)) {
            dir = Direction::targetting();
        }
    }

    while (!dir) {
        const auto prompt =
            target_okay(player_ptr)
                ? _("方向 ('5'でターゲットへ, '*'でターゲット再選択, ESCで中断)? ", "Direction ('5' for target, '*' to re-target, Escape to cancel)? ")
                : _("方向 ('*'でターゲット選択, ESCで中断)? ", "Direction ('*' to choose a target, Escape to cancel)? ");

        const auto command_opt = input_command(prompt, true);
        if (!command_opt) {
            project_length = 0;
            return Direction::none();
        }

        auto command = *command_opt;
        if (use_menu && (command == '\r')) {
            command = 't';
        }

        static const std::unordered_set select_new_target_commands = { '*', ' ', '\r' };
        static const std::unordered_set target_commands = { '*', ' ', '\r', 'T', 't', '.', '5', '0', '*' };
        if (target_commands.contains(command)) {
            if (select_new_target_commands.contains(command)) {
                target_set(player_ptr, TARGET_KILL);
            }
            if (target_okay(player_ptr)) {
                dir = Direction::targetting();
            }
        } else {
            dir = get_keymap_dir(command);
        }

        if (!dir) {
            bell();
        }
    }

    command_dir = dir;
    if (player_ptr->effects()->confusion().is_confused()) {
        dir = rand_choice(Direction::directions_8());
    }

    if (command_dir != dir) {
        msg_print(_("あなたは混乱している。", "You are confused."));
    }

    if (enable_repeat) {
        repeat_push(static_cast<short>(command_dir.dir()));
    }

    return dir;
}

/*!
 * @brief 上下左右および斜め方向を指定する
 * @return 指定した方向
 */
Direction get_direction(PlayerType *player_ptr)
{
    auto dir = command_dir;
    short code = 0;
    if (repeat_pull(&code) && Direction::is_valid_dir(code)) {
        dir = Direction(code);
    }

    constexpr auto prompt = _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
    while (!dir) {
        const auto command = input_command(prompt, true);
        if (!command) {
            return Direction::none();
        }

        dir = get_keymap_dir(*command);
        if (!dir) {
            bell();
        }
    }

    command_dir = dir;
    const auto finalizer = util::make_finalizer([] {
        repeat_push(static_cast<short>(command_dir.dir()));
    });
    const auto is_confused = player_ptr->effects()->confusion().is_confused();
    if (is_confused && evaluate_percent(75)) {
        dir = rand_choice(Direction::directions_8());
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
 * @brief 方向を指定する(騎乗対象の混乱の影響を受ける)
 *
 * 上下左右と斜めの8方向を指定する。underがtrueの場合、足元も指定可能。
 *
 * @param under 足元も指定可能かどうか
 * @return 指定した方向
 * @note この関数は繰り返し可能なコマンドに使用する。
 * (例) 走る、歩く、ドア開閉、ドア破壊、罠解除、楔を打つ、掘るなど
 */
Direction get_rep_dir(PlayerType *player_ptr, bool under)
{
    auto dir = command_dir;
    short code = 0;
    if (repeat_pull(&code) && Direction::is_valid_dir(code)) {
        dir = Direction(code);
    }

    const auto prompt = under ? _("方向 ('.'足元, ESCで中断)? ", "Direction ('.' at feet, Escape to cancel)? ")
                              : _("方向 (ESCで中断)? ", "Direction (Escape to cancel)? ");
    while (!dir) {
        const auto command = input_command(prompt, true);
        if (!command) {
            return Direction::none();
        }

        if (under && ((command == '5') || (command == '-') || (command == '.'))) {
            dir = Direction::self();
            break;
        }

        dir = get_keymap_dir(*command);
        if (!dir) {
            bell();
        }
    }

    if (dir == Direction::self() && !under) {
        return Direction::none();
    }

    command_dir = dir;
    auto is_confused = player_ptr->effects()->confusion().is_confused();
    if (is_confused) {
        if (evaluate_percent(75)) {
            dir = rand_choice(Direction::directions_8());
        }
    } else if (player_ptr->riding) {
        const auto &monster = player_ptr->current_floor_ptr->m_list[player_ptr->riding];
        const auto &monrace = monster.get_monrace();
        if (monster.is_confused()) {
            if (evaluate_percent(75)) {
                dir = rand_choice(Direction::directions_8());
            }
        } else if (monrace.behavior_flags.has_all_of({ MonsterBehaviorType::RAND_MOVE_50, MonsterBehaviorType::RAND_MOVE_25 }) && one_in_(2)) {
            dir = rand_choice(Direction::directions_8());
        } else if (monrace.behavior_flags.has(MonsterBehaviorType::RAND_MOVE_50) && one_in_(4)) {
            dir = rand_choice(Direction::directions_8());
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

    repeat_push(static_cast<short>(command_dir.dir()));
    return dir;
}
