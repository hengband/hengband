#include "cmd-visual/cmd-draw.h"
#include "core/asking-player.h"
#include "core/stuff-handler.h"
#include "core/window-redrawer.h"
#include "io/files-util.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "player-base/player-race.h"
#include "player-info/race-types.h"
#include "player/process-name.h"
#include "racial/racial-android.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "term/z-form.h"
#include "util/buffer-shaper.h"
#include "util/int-char-converter.h"
#include "util/string-processor.h"
#include "view/display-messages.h"
#include "view/display-player.h"
#include "world/world.h"
#include <optional>

/*!
 * @brief 画面を再描画するコマンドのメインルーチン
 * Hack -- redraw the screen
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details
 * <pre>
 * This command performs various low level updates, clears all the "extra"
 * windows, does a total redraw of the main window, and requests all of the
 * interesting updates and redraws that I can think of.
 *
 * This command is also used to "instantiate" the results of the user
 * selecting various things, such as graphics mode, so it must call
 * the "TERM_XTRA_REACT" hook before redrawing the windows.
 * </pre>
 */
void do_cmd_redraw(PlayerType *player_ptr)
{
    term_xtra(TERM_XTRA_REACT, 0);

    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_srf = {
        StatusRecalculatingFlag::COMBINATION,
        StatusRecalculatingFlag::REORDER,
        StatusRecalculatingFlag::TORCH,
        StatusRecalculatingFlag::BONUS,
        StatusRecalculatingFlag::HP,
        StatusRecalculatingFlag::MP,
        StatusRecalculatingFlag::SPELLS,
        StatusRecalculatingFlag::UN_VIEW,
        StatusRecalculatingFlag::UN_LITE,
        StatusRecalculatingFlag::VIEW,
        StatusRecalculatingFlag::LITE,
        StatusRecalculatingFlag::MONSTER_LITE,
        StatusRecalculatingFlag::MONSTER_STATUSES,
    };
    rfu.set_flags(flags_srf);
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    rfu.set_flags(flags_mwrf);
    static constexpr auto flags_swrf = {
        SubWindowRedrawingFlag::INVENTORY,
        SubWindowRedrawingFlag::EQUIPMENT,
        SubWindowRedrawingFlag::SPELL,
        SubWindowRedrawingFlag::PLAYER,
        SubWindowRedrawingFlag::MESSAGE,
        SubWindowRedrawingFlag::OVERHEAD,
        SubWindowRedrawingFlag::DUNGEON,
        SubWindowRedrawingFlag::MONSTER_LORE,
        SubWindowRedrawingFlag::ITEM_KNOWLEDGE,
    };
    rfu.set_flags(flags_swrf);
    AngbandWorld::get_instance().play_time.update();
    handle_stuff(player_ptr);
    if (PlayerRace(player_ptr).equals(PlayerRaceType::ANDROID)) {
        calc_android_exp(player_ptr);
    }

    term_type *old = game_term;
    for (auto i = 0U; i < angband_terms.size(); ++i) {
        if (!angband_terms[i]) {
            continue;
        }

        term_activate(angband_terms[i]);
        term_redraw();
        term_fresh();
        term_activate(old);
    }
}

static std::optional<int> input_status_command(PlayerType *player_ptr, int page)
{
    auto c = inkey();
    switch (c) {
    case 'c':
        get_name(player_ptr);
        process_player_name(player_ptr);
        return page;
    case 'f': {
        const auto initial_filename = format("%s.txt", player_ptr->base_name);
        const auto input_filename = input_string(_("ファイル名: ", "File name: "), 80, initial_filename);
        if (!input_filename) {
            return page;
        }

        const auto &filename = str_ltrim(*input_filename);
        if (!filename.empty()) {
            AngbandWorld::get_instance().play_time.update();
            file_character(player_ptr, filename);
        }

        return page;
    }
    case 'h':
        return page + 1;
    case ESCAPE:
        return std::nullopt;
    default:
        bell();
        return page;
    }
}

/*!
 * @brief プレイヤーのステータス表示
 */
void do_cmd_player_status(PlayerType *player_ptr)
{
    auto page = 0;
    screen_save();
    constexpr auto prompt = _("['c'で名前変更, 'f'でファイルへ書出, 'h'でモード変更, ESCで終了]", "['c' to change name, 'f' to file, 'h' to change mode, or ESC]");
    auto &world = AngbandWorld::get_instance();
    while (true) {
        TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

        world.play_time.update();
        (void)display_player(player_ptr, page);
        if (page == 5) {
            page = 0;
            (void)display_player(player_ptr, page);
        }

        term_putstr(2, 23, -1, TERM_WHITE, prompt);
        auto next_page = input_status_command(player_ptr, page);
        if (!next_page) {
            break;
        }

        page = *next_page;
        msg_erase();
    }

    screen_load();
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    static constexpr auto flags_mwrf = {
        MainWindowRedrawingFlag::WIPE,
        MainWindowRedrawingFlag::BASIC,
        MainWindowRedrawingFlag::EXTRA,
        MainWindowRedrawingFlag::EQUIPPY,
        MainWindowRedrawingFlag::MAP,
    };
    rfu.set_flags(flags_mwrf);
    handle_stuff(player_ptr);
}

/*!
 * @brief 最近表示されたメッセージを再表示するコマンドのメインルーチン
 * Recall the most recent message
 */
void do_cmd_message_one(void)
{
    prt(format("> %s", message_str(0)->data()), 0, 0);
}

/*!
 * @brief メッセージ履歴表示で最も古いメッセージを表示する時の基準メッセージ番号を計算する
 *
 * メッセージ履歴はウィンドウの幅によって表示行数が変わるため、最も古いメッセージを表示する時にいくつメッセージが
 * 表示できるかを実際に計算して基準メッセージ番号を求める.
 *
 * @param num_lines メッセージ表示行数
 * @param width ウィンドウの幅
 * @return 計算した基準メッセージ番号
 */
static int calc_oldest_base_msg_num(int num_lines, int width)
{
    auto lines_count = 0;

    for (auto oldest_base_msg_num = message_num(); oldest_base_msg_num > 0; --oldest_base_msg_num) {
        const auto msg_str = message_str(oldest_base_msg_num - 1);
        const auto lines = shape_buffer(*msg_str, width);
        lines_count += std::ssize(lines);
        if (lines_count > num_lines) {
            return oldest_base_msg_num;
        }
    }

    return 0;
}

/*!
 * @brief メッセージ履歴を表示する
 *
 * 基準メッセージ番号を指定して、メッセージ履歴を表示する。
 * メッセージの表示は基準メッセージ番号のものから古いほうへ、画面下から上に向かって行われる。
 * num_nowで指定したメッセージ番号より新しいメッセージは白で表示し、それ以外は灰色で表示する。
 * また、指定した文字列を黄色でハイライト表示する。
 *
 * @param base_msg_num 基準メッセージ番号
 * @param num_now 表示色を切り替える境界となるメッセージ番号
 * @param num_lines 表示行数
 * @param width ウィンドウの幅
 * @param shower ハイライト表示する文字列
 * @return 表示したメッセージ数
 */
static int display_message_history(int base_msg_num, int num_now, int num_lines, int width, const std::string &shower)
{
    auto displayed_lines = 0;

    int displayed_msg_count;
    for (displayed_msg_count = 0; (displayed_lines < num_lines); displayed_msg_count++) {
        const auto msg_num = base_msg_num + displayed_msg_count;
        if (msg_num >= message_num()) {
            break;
        }

        const auto msg_str = message_str(msg_num);
        const auto color = (msg_num < num_now) ? TERM_WHITE : TERM_SLATE;

        auto lines = shape_buffer(*msg_str, width);
        if (displayed_lines + std::ssize(lines) > num_lines) {
            break;
        }

        std::reverse(lines.begin(), lines.end());

        for (const auto &line : lines) {
            const auto y = num_lines + 1 - displayed_lines;
            c_prt(color, line, y, 0);
            displayed_lines++;

            if (shower.empty()) {
                continue;
            }

            /// @todo ハイライト表示する文字列の途中で折り返されると正しくハイライト表示されない。
            /// もともとメッセージ履歴自体を折り返して保存していた時も同様の問題があった。
            /// あまり使われていない機能と思われ、処理が複雑になるので、現状はそのままとしている。

            /// @note ダメ文字対策でstringを使わない.
            const auto *str = line.data();
            while (true) {
                str = angband_strstr(str, shower);
                if (str == nullptr) {
                    break;
                }

                const auto len = shower.length();
                term_putstr(str - line.data(), y, len, TERM_YELLOW, shower);
                str += len;
            }
        }
    }

    for (auto i = displayed_lines; i < num_lines; ++i) {
        term_erase(0, num_lines + 1 - i);
    }

    return displayed_msg_count;
}

/*!
 * @brief メッセージのログを表示するコマンドのメインルーチン
 * Recall the most recent message
 * @details
 * <pre>
 * Show previous messages to the user	-BEN-
 *
 * The screen format uses line 0 and 23 for headers and prompts,
 * skips line 1 and 22, and uses line 2 thru 21 for old messages.
 *
 * This command shows you which commands you are viewing, and allows
 * you to "search" for strings in the recall.
 *
 * Note that messages may be longer than 80 characters, but they are
 * displayed using "infinite" length, with a special sub-command to
 * "slide" the virtual display to the left or right.
 *
 * Attempt to only hilite the matching portions of the string.
 * </pre>
 */
void do_cmd_messages(int num_now)
{
    std::string shower_str("");
    std::string finder_str("");
    std::string shower("");
    const auto &[wid, hgt] = term_get_size();
    auto num_lines = hgt - 4;
    auto base_msg_num = 0;
    screen_save();
    term_clear();

    const auto oldest_base_msg_num = calc_oldest_base_msg_num(num_lines, wid);

    while (true) {
        const auto displayed_msg_count = display_message_history(base_msg_num, num_now, num_lines, wid, shower);

        constexpr auto fmt = _("以前のメッセージ %d-%d 全部で(%d)", "Message Recall (%d-%d of %d)");
        prt(format(fmt, base_msg_num, base_msg_num + displayed_msg_count - 1, message_num()), 0, 0);
        prt(_("[ 'p' で更に古いもの, 'n' で更に新しいもの, '/' で検索, ESC で中断 ]", "[Press 'p' for older, 'n' for newer, ..., or ESCAPE]"), hgt - 1, 0);

        const auto skey = inkey_special(true);
        if (skey == ESCAPE) {
            break;
        }

        const auto prev_base_msg_num = base_msg_num;
        switch (skey) {
        case '=': {
            prt(_("強調: ", "Show: "), hgt - 1, 0);
            const auto ask_result = askfor(80, shower_str);
            if (ask_result) {
                shower = *ask_result;
                shower_str = *ask_result;
            }

            continue;
        }
        case '/':
        case KTRL('s'): {
            prt(_("検索: ", "Find: "), hgt - 1, 0);
            const auto ask_result = askfor(80, finder_str);
            if (!ask_result) {
                continue;
            }

            finder_str = *ask_result;
            if (finder_str.empty()) {
                shower = "";
                continue;
            }

            shower = finder_str;
            for (auto msg_num = base_msg_num + 1; msg_num < message_num(); msg_num++) {
                // @details ダメ文字対策でstringを使わない.
                const auto msg = message_str(msg_num);
                if (str_find(*msg, finder_str)) {
                    base_msg_num = msg_num;
                    break;
                }
            }

            break;
        }
        case SKEY_HOME:
            base_msg_num = oldest_base_msg_num;
            break;
        case SKEY_END:
            base_msg_num = 0;
            break;
        case '8':
        case SKEY_UP:
        case '\n':
        case '\r':
            base_msg_num = std::min(base_msg_num + 1, oldest_base_msg_num);
            break;
        case '+':
            base_msg_num = std::min(base_msg_num + 10, oldest_base_msg_num);
            break;
        case 'p':
        case KTRL('P'):
        case ' ':
        case SKEY_PGUP:
            base_msg_num = std::min(base_msg_num + displayed_msg_count, oldest_base_msg_num);
            break;
        case 'n':
        case KTRL('N'):
        case SKEY_PGDOWN:
            base_msg_num = std::max(0, base_msg_num - displayed_msg_count);
            break;
        case '-':
            base_msg_num = std::max(0, base_msg_num - 10);
            break;
        case '2':
        case SKEY_DOWN:
            base_msg_num = std::max(0, base_msg_num - 1);
            break;
        }

        if (base_msg_num == prev_base_msg_num) {
            bell();
        }
    }

    screen_load();
}
