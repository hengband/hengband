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
    update_playtime();
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
            update_playtime();
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
    while (true) {
        TermCenteredOffsetSetter tcos(MAIN_TERM_MIN_COLS, MAIN_TERM_MIN_ROWS);

        update_playtime();
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
    auto n = message_num();
    auto i = 0;
    screen_save();
    term_clear();
    while (true) {
        int j;
        int skey;
        for (j = 0; (j < num_lines) && (i + j < n); j++) {
            const auto msg_str = message_str(i + j);
            const auto *msg = msg_str->data();
            c_prt((i + j < num_now ? TERM_WHITE : TERM_SLATE), msg, num_lines + 1 - j, 0);
            if (shower.empty()) {
                continue;
            }

            // @details ダメ文字対策でstringを使わない.
            const auto *str = msg;
            while (true) {
                str = angband_strstr(str, shower);
                if (str == nullptr) {
                    break;
                }

                const auto len = shower.length();
                term_putstr(str - msg, num_lines + 1 - j, len, TERM_YELLOW, shower);
                str += len;
            }
        }

        for (; j < num_lines; j++) {
            term_erase(0, num_lines + 1 - j);
        }

        prt(format(_("以前のメッセージ %d-%d 全部で(%d)", "Message Recall (%d-%d of %d)"), i, i + j - 1, n), 0, 0);
        prt(_("[ 'p' で更に古いもの, 'n' で更に新しいもの, '/' で検索, ESC で中断 ]", "[Press 'p' for older, 'n' for newer, ..., or ESCAPE]"), hgt - 1, 0);
        skey = inkey_special(true);
        if (skey == ESCAPE) {
            break;
        }

        j = i;
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
            for (int z = i + 1; z < n; z++) {
                // @details ダメ文字対策でstringを使わない.
                const auto msg = message_str(z);
                if (str_find(*msg, finder_str)) {
                    i = z;
                    break;
                }
            }

            break;
        }
        case SKEY_TOP:
            i = n - num_lines;
            break;
        case SKEY_BOTTOM:
            i = 0;
            break;
        case '8':
        case SKEY_UP:
        case '\n':
        case '\r':
            i = std::min(i + 1, n - num_lines);
            break;
        case '+':
            i = std::min(i + 10, n - num_lines);
            break;
        case 'p':
        case KTRL('P'):
        case ' ':
        case SKEY_PGUP:
            i = std::min(i + num_lines, n - num_lines);
            break;
        case 'n':
        case KTRL('N'):
        case SKEY_PGDOWN:
            i = std::max(0, i - num_lines);
            break;
        case '-':
            i = std::max(0, i - 10);
            break;
        case '2':
        case SKEY_DOWN:
            i = std::max(0, i - 1);
            break;
        }

        if (i == j) {
            bell();
        }
    }

    screen_load();
}
