#include "birth/history-editor.h"
#include "core/asking-player.h"
#include "io/input-context.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "locale/multibyte-char.h"
#include "player/player-history.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "view/display-player.h" // 暫定。後で消す.
#include <algorithm>

namespace {
/*!
 * @brief 行の表示桁にある文字の先頭のバイト位置を返す
 * @param line 行
 * @param column 表示桁
 * @return 文字の先頭のバイト位置。column が全角文字の右半分なら、その文字の先頭
 */
size_t byte_pos_at_column(std::string_view line, int column)
{
    return str_prefix_by_width(line, column).length();
}

/*!
 * @brief 生い立ちの各行を表示幅 PLAYER_HISTORY_WIDTH 桁まで空白で埋める
 * @param history 生い立ち
 * @details 編集中はどの桁にも文字があるように埋めておく。
 */
void pad_history_lines(PlayerHistory &history)
{
    constexpr auto max_width = static_cast<size_t>(PLAYER_HISTORY_WIDTH);
    for (auto &line : history) {
        const auto width = str_display_width(line);
        if (width < max_width) {
            line.append(max_width - width, ' ');
        }
    }
}
}

/*!
 * @brief 行の表示桁にある文字の先頭の表示桁を返す
 * @param line 行
 * @param column 表示桁
 * @return 文字の先頭の表示桁。column が全角文字の右半分なら column - 1。column が行の表示幅を超えていれば行の表示幅
 * @details 編集中の行は表示幅 PLAYER_HISTORY_WIDTH 桁まで空白で埋めてあるので、カーソルの桁は行の中に収まる。
 */
int history_char_start_column(std::string_view line, int column)
{
    return static_cast<int>(str_display_width(str_prefix_by_width(line, column)));
}

/*!
 * @brief 行の表示桁にある文字を返す
 * @param line 行
 * @param column 表示桁
 * @return 文字。column が行の表示幅を超えていれば空
 */
std::string_view history_char_at_column(std::string_view line, int column)
{
    return mb_first_char(line.substr(byte_pos_at_column(line, column)));
}

/*!
 * @brief 行の表示桁に文字を上書きする
 * @param line 行
 * @param column 文字を書く表示桁
 * @param ch 書く文字
 * @details 書く文字の幅の分だけ、column から後ろの文字を取り除いてから書く。
 * 取り除いた全角文字が書く文字からはみ出していた分は空白にする。
 * 行が column + 書く文字の幅まで届いていれば行の表示幅は変わらないが、届いていなければその分だけ長くなる。
 * 編集中は行を空白で埋め、行末に収まらない全角文字は次の行に送ってから呼ぶので、行の表示幅は変わらない。
 */
void overwrite_history_char(std::string &line, int column, std::string_view ch)
{
    const auto pos = byte_pos_at_column(line, column);
    const auto char_width = str_display_width(ch);
    auto end = pos;
    size_t removed_width = 0;
    while ((end < line.length()) && (removed_width < char_width)) {
        const auto removed = mb_first_char(std::string_view(line).substr(end));
        removed_width += str_display_width(removed);
        end += removed.length();
    }

    std::string replacement(ch);
    if (removed_width > char_width) {
        replacement.append(removed_width - char_width, ' ');
    }

    line.replace(pos, end - pos, replacement);
}

/*!
 * @brief 生い立ちメッセージを編集する。/Character background edit-mode
 * @param player_ptr プレイヤーへの参照ポインタ
 * @details カーソルの位置 x は行の中の表示桁で持ち、常に文字の先頭に置く。
 */
void edit_history(PlayerType *player_ptr)
{
    const auto old_history = player_ptr->history;
    pad_history_lines(player_ptr->history);

    (void)display_player(player_ptr, 1);
    c_put_str(TERM_L_GREEN, _("(キャラクターの生い立ち - 編集モード)", "(Character Background - Edit Mode)"), 11, 20);
    constexpr auto prompt = _("[ カーソルキーで移動、Enterで終了、Ctrl-Aでファイル読み込み ]", "[ Cursor key for Move, Enter for End, Ctrl-A for Read pref ]");
    put_str(prompt, 17, 10);
    constexpr auto last_line = PLAYER_HISTORY_LINES - 1;
    constexpr auto last_column = PLAYER_HISTORY_WIDTH - 1;
    TERM_LEN y = 0;
    TERM_LEN x = 0;
    while (true) {
        char c;

        for (int i = 0; i < PLAYER_HISTORY_LINES; i++) {
            put_str(player_ptr->history[i], i + 12, 10);
        }

        c_put_str(TERM_L_BLUE, history_char_at_column(player_ptr->history[y], x), y + 12, x + 10);

        term_gotoxy(x + 10, y + 12);
        const auto skey = input_special_key_with_context({ InputKind::BIRTH, prompt, BirthInputDetails{ BirthStep::HISTORY } });
        if (!(skey & SKEY_MASK)) {
            c = (char)skey;
        } else {
            c = 0;
        }

        if (skey == SKEY_UP || c == KTRL('p')) {
            y--;
            if (y < 0) {
                y = last_line;
            }

            x = history_char_start_column(player_ptr->history[y], x);
        } else if (skey == SKEY_DOWN || c == KTRL('n')) {
            y++;
            if (y > last_line) {
                y = 0;
            }

            x = history_char_start_column(player_ptr->history[y], x);
        } else if (skey == SKEY_RIGHT || c == KTRL('f')) {
            // 行は空白で埋めてあるので文字は必ずあるが、念のため少なくとも 1 桁は進める
            x += std::max(static_cast<int>(str_display_width(history_char_at_column(player_ptr->history[y], x))), 1);
            if (x > last_column) {
                x = 0;
                if (y < last_line) {
                    y++;
                }
            }
        } else if (skey == SKEY_LEFT || c == KTRL('b')) {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = last_column;
                } else {
                    x = 0;
                }
            }

            x = history_char_start_column(player_ptr->history[y], x);
        } else if (c == '\r' || c == '\n') {
            term_erase(0, 11);
            term_erase(0, 17);
            put_str(_("(キャラクターの生い立ち - 編集済み)", "(Character Background - Edited)"), 11, 20);
            break;
        } else if (c == ESCAPE) {
            clear_from(11);
            put_str(_("(キャラクターの生い立ち)", "(Character Background)"), 11, 25);
            player_ptr->history = old_history;
            for (int i = 0; i < PLAYER_HISTORY_LINES; i++) {
                put_str(player_ptr->history[i], i + 12, 10);
            }

            break;
        } else if (c == KTRL('A')) {
            if (read_histpref(player_ptr)) {
                pad_history_lines(player_ptr->history);
                x = history_char_start_column(player_ptr->history[y], x);
            }
        } else if (c == '\010') {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = last_column;
                } else {
                    x = 0;
                }
            }

            x = history_char_start_column(player_ptr->history[y], x);
            overwrite_history_char(player_ptr->history[y], x, " ");
        } else if (is_mb_char_lead(c) || isprint(static_cast<unsigned char>(c))) {
            // 入力が途中で切れて文字にならなければ何も書かない
            const auto ch_opt = inkey_multibyte_char(c);
            if (!ch_opt) {
                continue;
            }

            const auto &ch = *ch_opt;

            const auto char_width = static_cast<int>(str_display_width(ch));

            // 行末に収まらない全角文字は、次の行の先頭に書く
            if (x + char_width > PLAYER_HISTORY_WIDTH) {
                x = 0;
                y++;
                if (y > last_line) {
                    y = 0;
                }
            }

            overwrite_history_char(player_ptr->history[y], x, ch);
            x += char_width;
            if (x > last_column) {
                x = 0;
                y++;
                if (y > last_line) {
                    y = 0;
                }
            }
        }
    }
}
