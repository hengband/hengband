#include "birth/history-editor.h"
#include "core/asking-player.h"
#include "io/input-context.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "locale/character-encoding.h"
#include "locale/multibyte-char.h"
#include "player/player-history.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "view/display-player.h" // 暫定。後で消す.

namespace {
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
 * @brief 生い立ちメッセージを編集する。/Character background edit-mode
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void edit_history(PlayerType *player_ptr)
{
    const auto old_history = player_ptr->history;
    pad_history_lines(player_ptr->history);

    (void)display_player(player_ptr, 1);
    c_put_str(TERM_L_GREEN, _("(キャラクターの生い立ち - 編集モード)", "(Character Background - Edit Mode)"), 11, 20);
    constexpr auto prompt = _("[ カーソルキーで移動、Enterで終了、Ctrl-Aでファイル読み込み ]", "[ Cursor key for Move, Enter for End, Ctrl-A for Read pref ]");
    put_str(prompt, 17, 10);
    TERM_LEN y = 0;
    TERM_LEN x = 0;
    while (true) {
        char c;

        for (int i = 0; i < PLAYER_HISTORY_LINES; i++) {
            put_str(player_ptr->history[i], i + 12, 10);
        }
#ifdef JP
        const auto char_len = iskanji2(player_ptr->history[y].data(), x) ? 2 : 1;
#else
        const auto char_len = 1;
#endif
        c_put_str(TERM_L_BLUE, std::string_view(&player_ptr->history[y][x], char_len), y + 12, x + 10);

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
                y = 3;
            }
#ifdef JP
            if ((x > 0) && (iskanji2(player_ptr->history[y].data(), x - 1))) {
                x--;
            }
#endif
        } else if (skey == SKEY_DOWN || c == KTRL('n')) {
            y++;
            if (y > 3) {
                y = 0;
            }
#ifdef JP
            if ((x > 0) && (iskanji2(player_ptr->history[y].data(), x - 1))) {
                x--;
            }
#endif
        } else if (skey == SKEY_RIGHT || c == KTRL('f')) {
#ifdef JP
            if (iskanji2(player_ptr->history[y].data(), x)) {
                x++;
            }
#endif
            x++;
            if (x > 58) {
                x = 0;
                if (y < 3) {
                    y++;
                }
            }
        } else if (skey == SKEY_LEFT || c == KTRL('b')) {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else {
                    x = 0;
                }
            }

#ifdef JP
            if ((x > 0) && (iskanji2(player_ptr->history[y].data(), x - 1))) {
                x--;
            }
#endif
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
#ifdef JP
                if ((x > 0) && (iskanji2(player_ptr->history[y].data(), x - 1))) {
                    x--;
                }
#endif
            }
        } else if (c == '\010') {
            x--;
            if (x < 0) {
                if (y) {
                    y--;
                    x = 58;
                } else {
                    x = 0;
                }
            }

            player_ptr->history[y][x] = ' ';
#ifdef JP
            if ((x > 0) && (iskanji2(player_ptr->history[y].data(), x - 1))) {
                x--;
                player_ptr->history[y][x] = ' ';
            }
#endif
        }
#ifdef JP
        else if (iskanji(c) || isprint(c))
#else
        else if (isprint(c)) /* BUGFIX */
#endif
        {
#ifdef JP
            auto trail = '\0';
            if (iskanji(c)) {
                // 後続のバイトに ASCII の制御文字が来たら (入力が途中で切れたなど)、行に NUL などを入れないよう文字ごと捨てる。
                // iscntrl() は Windows の日本語のロケールで Shift_JIS の後続のバイトの 0x80 も制御文字とみなすので使わない
                trail = inkey();
                const auto trail_byte = static_cast<unsigned char>(trail);
                if ((trail_byte < 0x20) || (trail_byte == 0x7f)) {
                    continue;
                }
            }

            if (iskanji2(player_ptr->history[y].data(), x)) {
                player_ptr->history[y][x + 1] = ' ';
            }

            if (iskanji(c)) {
                if (x > 57) {
                    x = 0;
                    y++;
                    if (y > 3) {
                        y = 0;
                    }
                }

                if (iskanji2(player_ptr->history[y].data(), x + 1)) {
                    player_ptr->history[y][x + 2] = ' ';
                }

                player_ptr->history[y][x++] = c;

                c = trail;
            }
#endif
            player_ptr->history[y][x++] = c;
            if (x > 58) {
                x = 0;
                y++;
                if (y > 3) {
                    y = 0;
                }
            }
        }
    }
}
