#include "cmd-io/cmd-floor.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "floor/geometry.h"
#include "game-option/keymap-directory-getter.h"
#include "io/cursor.h"
#include "io/screen-util.h"
#include "main/sound-of-music.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "target/target-setter.h"
#include "target/target-types.h"
#include "term/z-form.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"

/*!
 * @brief ターゲットを設定するコマンドのメインルーチン
 * Target command
 */
void do_cmd_target(PlayerType *player_ptr)
{
    if (player_ptr->wild_mode) {
        return;
    }

    if (target_set(player_ptr, TARGET_KILL)) {
        msg_print(_("ターゲット決定。", "Target Selected."));
    } else {
        msg_print(_("ターゲット解除。", "Target Aborted."));
    }
}

/*!
 * @brief 周囲を見渡すコマンドのメインルーチン
 * Look command
 */
void do_cmd_look(PlayerType *player_ptr)
{
    static constexpr auto flags = {
        SubWindowRedrawingFlag::SIGHT_MONSTERS,
        SubWindowRedrawingFlag::FLOOR_ITEMS,
    };
    RedrawingFlagsUpdater::get_instance().set_flags(flags);
    handle_stuff(player_ptr);
    if (target_set(player_ptr, TARGET_LOOK)) {
        msg_print(_("ターゲット決定。", "Target Selected."));
    }
}

/*!
 * @brief 位置を確認するコマンドのメインルーチン
 * Allow the player to examine other sectors on the map
 */
void do_cmd_locate(PlayerType *player_ptr)
{
    static constexpr std::array<std::array<std::string_view, 3>, 3> dirstrings = { {
        { { _("北西", " northwest of"), _("北", " north of"), _("北東", " northeast of") } },
        { { _("西", " west of"), _("真上", ""), _("東", " east of") } },
        { { _("南西", " southwest of"), _("南", " south of"), _("南東", " southeast of") } },
    } };
    POSITION y1, x1;
    TERM_LEN wid, hgt;
    get_screen_size(&wid, &hgt);
    POSITION y2 = y1 = panel_row_min;
    POSITION x2 = x1 = panel_col_min;
    constexpr auto fmt = _("マップ位置 [%d(%02d),%d(%02d)] (プレイヤーの%s)  方向?", "Map sector [%d(%02d),%d(%02d)], which is%s your sector.  Direction?");
    while (true) {
        const auto &dirstring = dirstrings[(y2 < y1) ? 0 : ((y2 > y1) ? 2 : 1)][(x2 < x1) ? 0 : ((x2 > x1) ? 2 : 1)];
        const auto prompt = format(fmt, y2 / (hgt / 2), y2 % (hgt / 2), x2 / (wid / 2), x2 % (wid / 2), dirstring.data());
        auto dir = 0;
        while (dir == 0) {
            const auto command = input_command(prompt, true);
            if (!command.has_value()) {
                break;
            }

            dir = get_keymap_dir(command.value());
            if (dir == 0) {
                bell();
            }
        }

        if (dir == 0) {
            break;
        }

        if (change_panel(player_ptr, ddy[dir], ddx[dir])) {
            y2 = panel_row_min;
            x2 = panel_col_min;
        }
    }

    verify_panel(player_ptr);
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    rfu.set_flag(StatusRecalculatingFlag::MONSTER_STATUSES);
    rfu.set_flag(MainWindowRedrawingFlag::MAP);
    static constexpr auto flags = {
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
    };
    rfu.set_flags(flags);
    handle_stuff(player_ptr);
}
