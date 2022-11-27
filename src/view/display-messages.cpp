#include "view/display-messages.h"
#include "core/window-redrawer.h"
#include "game-option/cheat-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/option-flags.h"
#include "io/input-key-acceptor.h"
#include "main/sound-of-music.h"
#include "system/player-type-definition.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/int-char-converter.h"
#include "world/world.h"

#include <deque>
#include <map>
#include <memory>
#include <string>

/* Used in msg_print() for "buffering" */
bool msg_flag;

COMMAND_CODE now_message;

namespace {

/*! 表示するメッセージの先頭位置 */
static int msg_head_pos = 0;

using msg_sp = std::shared_ptr<const std::string>;
using msg_wp = std::weak_ptr<const std::string>;

/** メッセージが同一かどうかを比較するためのラムダ式 */
auto string_ptr_cmp = [](const std::string *a, const std::string *b) { return *a < *b; };

/** 同一メッセージの検索に使用するmapオブジェクト。
 * 同一メッセージがあるかどうかを検索し、ヒットしたらweak_ptrからshared_ptrを作成しメッセージを共有する。
 * message_historyのカスタムデリータの中で参照するので、message_historyより先に宣言しなければならない事に注意。
 */
std::map<const std::string *, msg_wp, decltype(string_ptr_cmp)> message_map(string_ptr_cmp);

/** メッセージ履歴 */
std::deque<msg_sp> message_history;

/**
 * @brief メッセージを保持する msg_sp オブジェクトを生成する
 *
 * @tparam T メッセージの型。std::string / std::string_view / const char* 等
 * @param str メッセージ
 * @return 生成した msg_sp オブジェクト
 */
template <typename T>
msg_sp make_message(T &&str)
{
    /** std::stringオブジェクトと同時にmessage_mapのエントリも削除するカスタムデリータ */
    auto deleter = [](std::string *s) {
        message_map.erase(s);
        delete s;
    };

    // 新たにメッセージを保持する msg_sp オブジェクトを生成し、検索用mapオブジェクトにも追加する
    auto new_msg = msg_sp(new std::string(std::forward<T>(str)), std::move(deleter));
    message_map.emplace(new_msg.get(), msg_wp(new_msg));

    return new_msg;
}
}

/*!
 * @brief 保存中の過去ゲームメッセージの数を返す。 / How many messages are "available"?
 * @return 残っているメッセージの数
 */
int32_t message_num(void)
{
    return message_history.size();
}

/*!
 * @brief 過去のゲームメッセージを返す。 / Recall the "text" of a saved message
 * @param age メッセージの世代
 * @return メッセージの文字列ポインタ
 */
concptr message_str(int age)
{
    if ((age < 0) || (age >= message_num())) {
        return "";
    }

    return message_history[age]->data();
}

static void message_add_aux(std::string str)
{
    std::string splitted;

    if (str.empty()) {
        return;
    }

    // 80桁を超えるメッセージは80桁ずつ分割する
    if (str.length() > 80) {
        int n;
#ifdef JP
        for (n = 0; n < 80; n++) {
            if (iskanji(str[n])) {
                n++;
            }
        }

        /* 最後の文字が漢字半分 */
        if (n == 81) {
            n = 79;
        }
#else
        for (n = 80; n > 60; n--) {
            if (str[n] == ' ') {
                break;
            }
        }
        if (n == 60) {
            n = 80;
        }
#endif
        splitted = str.substr(n);
        str = str.substr(0, n);
    }

    // 直前と同じメッセージの場合、「～ <xNN>」と表示する
    if (!message_history.empty()) {
        const char *t;
        std::string_view last_message = *message_history.front();
#ifdef JP
        for (t = last_message.data(); *t && (*t != '<' || (*(t + 1) != 'x')); t++) {
            if (iskanji(*t)) {
                t++;
            }
        }
#else
        for (t = last_message.data(); *t && (*t != '<'); t++) {
            ;
        }
#endif
        int j = 1;
        if (*t && t != last_message.data()) {
            if (last_message.length() >= sizeof(" <xN>") - 1) {
                last_message = last_message.substr(0, t - last_message.data() - 1);
                j = atoi(t + 2);
            }
        }

        if (str == last_message && (j < 1000)) {
            str = format("%s <x%d>", str.data(), j + 1);
            message_history.pop_front();
            if (!now_message) {
                now_message++;
            }
        } else {
            /*流れた行の数を数えておく */
            num_more++;
            now_message++;
        }
    }

    msg_sp add_msg;

    // メッセージ履歴から同一のメッセージを探す
    if (const auto &it = message_map.find(&str); it != message_map.end()) {
        // 同一のメッセージが見つかったならそのメッセージの msg_sp オブジェクトを複製
        add_msg = it->second.lock();
    } else {
        // 見つからなかった場合は新たに msg_sp オブジェクトを作成
        add_msg = make_message(std::move(str));
    }

    // メッセージ履歴に追加
    message_history.push_front(std::move(add_msg));

    if (message_history.size() == MESSAGE_MAX) {
        message_history.pop_back();
    }

    if (!splitted.empty()) {
        message_add_aux(std::move(splitted));
    }
}

/*!
 * @brief ゲームメッセージをログに追加する。 / Add a new message, with great efficiency
 * @param msg 保存したいメッセージ
 */
void message_add(std::string_view msg)
{
    message_add_aux(std::string(msg));
}

bool is_msg_window_flowed(void)
{
    auto i = 0U;
    for (; i < angband_terms.size(); ++i) {
        if (angband_terms[i] && (window_flag[i] & PW_MESSAGE)) {
            break;
        }
    }
    if (i < 8) {
        if (num_more < angband_terms[i]->hgt) {
            return false;
        }

        return num_more >= 0;
    }
    return num_more >= 0;
}

/*
 * Hack -- flush
 */
static void msg_flush(PlayerType *player_ptr, int x)
{
    byte a = TERM_L_BLUE;
    bool show_more = (num_more >= 0);

    if (auto_more && !player_ptr->now_damaged) {
        show_more = is_msg_window_flowed();
    }

    if (skip_more) {
        show_more = false;
    }

    player_ptr->now_damaged = false;
    if (!player_ptr->playing || show_more) {
        term_putstr(x, 0, -1, a, _("-続く-", "-more-"));
        while (true) {
            int cmd = inkey();
            if (cmd == ESCAPE) {
                /* auto_moreのとき、全て流す */
                num_more = -9999;
                break;
            } else if (cmd == ' ') {
                /* 1画面だけ流す */
                num_more = 0;
                break;
            } else if ((cmd == '\n') || (cmd == '\r')) {
                /* 1行だけ流す */
                num_more--;
                break;
            }

            if (quick_messages) {
                break;
            }
            bell();
        }
    }

    term_erase(0, 0, 255);
}

void msg_erase(void)
{
    msg_print(nullptr);
}

static int split_length(std::string_view sv, int max)
{
    auto split = max;

#ifdef JP
    auto k_flag = false;
    auto wordlen = 0;
    for (auto check = 0; check < max; check++) {
        if (k_flag) {
            k_flag = false;
            continue;
        }

        if (iskanji(sv[check])) {
            k_flag = true;
            split = check;
        } else if (sv[check] == ' ') {
            split = check;
            wordlen = 0;
        } else {
            wordlen++;
            if (wordlen > 20) {
                split = check;
            }
        }
    }
#else
    for (auto check = 40; check < 72; check++) {
        if (sv[check] == ' ') {
            split = check;
        }
    }
#endif

    return split;
}

/*!
 * @briefOutput a message to the top line of the screen.
 *
 * Break long messages into multiple pieces (40-72 chars).
 *
 * Allow multiple short messages to "share" the top line.
 *
 * Prompt the user to make sure he has a chance to read them.
 *
 * These messages are memorized for later reference (see above).
 *
 * We could do "term_fresh()" to provide "flicker" if needed.
 *
 * The global "msg_flag" variable can be cleared to tell us to
 * "erase" any "pending" messages still on the screen.
 *
 * Note that we must be very careful about using the
 * "msg_print()" functions without explicitly calling the special
 * "msg_print(nullptr)" function, since this may result in the loss
 * of information if the screen is cleared, or if anything is
 * displayed on the top line.
 *
 * Note that "msg_print(nullptr)" will clear the top line
 * even if no messages are pending.  This is probably a hack.
 * @todo ここのp_ptrを削除するのは破滅的に作業が増えるので保留
 */
void msg_print(std::string_view msg)
{
    if (w_ptr->timewalk_m_idx) {
        return;
    }

    if (!msg_flag) {
        term_erase(0, 0, 255);
        msg_head_pos = 0;
    }

    std::string msg_includes_turn;
    if (cheat_turn) {
        msg = msg_includes_turn = format("T:%d - %s", w_ptr->game_turn, msg.data());
    }

    if ((msg_head_pos > 0) && ((msg_head_pos + msg.size()) > 72)) {
        msg_flush(p_ptr, msg_head_pos);
        msg_flag = false;
        msg_head_pos = 0;
    }

    if (msg.size() > 1000) {
        return;
    }

    if (w_ptr->character_generated) {
        message_add(msg);
    }

    while (msg.size() > 72) {
        auto split = split_length(msg, 72);
        term_putstr(0, 0, split, TERM_WHITE, msg.data());
        msg_flush(p_ptr, split + 1);
        msg.remove_prefix(split);
    }

    term_putstr(msg_head_pos, 0, msg.size(), TERM_WHITE, msg.data());
    p_ptr->window_flags |= (PW_MESSAGE);
    window_stuff(p_ptr);

    msg_flag = true;
    msg_head_pos += msg.size() + _(0, 1);

    if (fresh_message) {
        term_fresh_force();
    }
}

void msg_print(std::nullptr_t)
{
    if (w_ptr->timewalk_m_idx) {
        return;
    }

    if (!msg_flag) {
        term_erase(0, 0, 255);
        msg_head_pos = 0;
    }

    if (msg_head_pos > 0) {
        msg_flush(p_ptr, msg_head_pos);
        msg_flag = false;
        msg_head_pos = 0;
    }
}

/*
 * Display a formatted message, using "vstrnfmt()" and "msg_print()".
 */
void msg_format(std::string_view fmt, ...)
{
    va_list vp;
    char buf[1024];
    va_start(vp, fmt);
    (void)vstrnfmt(buf, sizeof(buf), fmt.data(), vp);
    va_end(vp);
    msg_print(buf);
}
