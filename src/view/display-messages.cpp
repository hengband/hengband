#include "view/display-messages.h"
#include "core/window-redrawer.h"
#include "game-option/cheat-options.h"
#include "game-option/input-options.h"
#include "game-option/map-screen-options.h"
#include "game-option/option-flags.h"
#include "game-option/text-display-options.h"
#include "io/input-key-acceptor.h"
#include "load/load-util.h"
#include "main/sound-of-music.h"
#include "save/save-util.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "util/finalizer.h"
#include "util/int-char-converter.h"
#include "world/world.h"
#include <algorithm>
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

/*! -続く- を表示してキー入力を待っている最中か否か */
static bool waiting_for_more = false;

/*! キー入力を待っている -続く- を表示したい位置 */
static int more_prompt_x = 0;

/*!
 * @brief メッセージ行 (画面の1行目) に表示しているメッセージ
 * @details -続く- の待ちの最中に端末の大きさが変わった時、-続く- で上書きした部分を含めて描き直すために覚えておく
 */
static std::string msg_line;

constexpr std::string_view MORE_PROMPT = _("-続く-", "-more-");

/*! 起動してから履歴に追加したメッセージ行の累計 (繰り返しによる回数の加算は含まない) */
static uint64_t message_sequence_count = 0;

using msg_sp = std::shared_ptr<const std::string>;
using msg_wp = std::weak_ptr<const std::string>;

/** メッセージが同一かどうかを比較するためのラムダ式 */
auto string_ptr_cmp = [](const std::string *a, const std::string *b) { return *a < *b; };

/** 同一メッセージの検索に使用するmapオブジェクト。
 * 同一メッセージがあるかどうかを検索し、ヒットしたらweak_ptrからshared_ptrを作成しメッセージを共有する。
 * message_historyのカスタムデリータの中で参照するので、message_historyより先に宣言しなければならない事に注意。
 */
std::map<const std::string *, msg_wp, decltype(string_ptr_cmp)> message_map(string_ptr_cmp);

/** メッセージ行 */
struct msg_record {
    msg_record(msg_sp msg, short repeat_count = 1)
        : msg(std::move(msg))
        , repeat_count(repeat_count)
    {
    }
    msg_sp msg; //< メッセージ
    short repeat_count; //< 繰り返し回数
};

/** メッセージ履歴 */
std::deque<msg_record> message_history;

/**
 * @brief メッセージを保持する msg_sp オブジェクトを生成する
 *
 * @param msg メッセージ
 * @return 生成した msg_sp オブジェクト
 */
msg_sp make_message(std::string &&msg)
{
    // メッセージ履歴に同一のメッセージがあれば、そのメッセージの msg_sp オブジェクトを複製して返す
    if (const auto &it = message_map.find(&msg); it != message_map.end()) {
        return it->second.lock();
    }

    // 見つからなければ新たに msg_sp オブジェクトを作成する
    /** std::stringオブジェクトと同時にmessage_mapのエントリも削除するカスタムデリータ */
    auto deleter = [](std::string *s) {
        message_map.erase(s);
        delete s;
    };

    // 新たにメッセージを保持する msg_sp オブジェクトを生成し、検索用mapオブジェクトにも追加する
    auto new_msg = msg_sp(new std::string(std::move(msg)), std::move(deleter));
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
 * @brief 起動してから履歴に追加したメッセージ行の累計を返す
 * @return 累計の行数
 * @details 履歴の件数は上限 (MESSAGE_MAX) に達すると増えなくなるので、新着の数え上げにはこちらを使う。
 * 同じメッセージの繰り返しは既存の行の回数表示 (<xN>) を増やすだけなので数えない。
 */
uint64_t message_sequence()
{
    return message_sequence_count;
}

/*!
 * @brief 過去のゲームメッセージを返す。 / Recall the "text" of a saved message
 * @param age メッセージの世代
 * @return メッセージの文字列ポインタ
 */
std::shared_ptr<const std::string> message_str(int age)
{
    if ((age < 0) || (age >= message_num())) {
        return std::make_shared<const std::string>("");
    }

    const auto &[msg, repeat_count] = message_history[age];
    if (repeat_count > 1) {
        return std::make_shared<const std::string>(*msg + format(" <x%d>", repeat_count));
    }

    return msg;
}

/*!
 * @brief メッセージ履歴にメッセージを追加する
 * @param msg 保存するメッセージ
 */
void message_add(std::string_view msg)
{
    if (msg.empty()) {
        return;
    }

    if (!message_history.empty()) {
        auto &last_msg = message_history.front();

        // 直前と同じメッセージの場合、繰り返し回数を増やして終了
        if ((msg == *last_msg.msg) && (last_msg.repeat_count < 9999)) {
            last_msg.repeat_count++;
            if (!now_message) {
                now_message++;
            }
            return;
        }

        /*流れた行の数を数えておく */
        num_more++;
        now_message++;
    }

    // メッセージ履歴に追加
    message_history.emplace_front(make_message(std::string(msg)));
    message_sequence_count++;

    while (message_history.size() > MESSAGE_MAX) {
        message_history.pop_back();
    }
}

bool is_msg_window_flowed(void)
{
    auto i = 0U;
    for (; i < angband_terms.size(); ++i) {
        if (angband_terms[i] && g_window_flags[i].has(SubWindowRedrawingFlag::MESSAGE)) {
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

/*!
 * @brief メッセージ行を消去する
 */
static void erase_msg_line()
{
    term_erase(0, 0);
    msg_line.clear();
}

/*!
 * @brief メッセージ行にメッセージを表示する
 * @param x 表示する位置
 * @param str 表示するメッセージ
 */
static void put_msg_line(int x, std::string_view str)
{
    term_putstr(x, 0, std::ssize(str), TERM_WHITE, str);
    msg_line.resize(x, ' ');
    msg_line.append(str);
}

/*!
 * @brief -続く- を表示する
 * @param x 表示したい位置
 * @details 端末の幅に収まらない位置であれば、右端に収まるよう左へ寄せる。
 */
static void put_more_prompt(int x)
{
    const auto &[wid, hgt] = term_get_size();
    term_putstr(std::min<int>(x, wid - std::ssize(MORE_PROMPT)), 0, -1, TERM_L_BLUE, MORE_PROMPT);
}

/*
 * Hack -- flush
 */
static void msg_flush(PlayerType *player_ptr, int x)
{
    bool show_more = (num_more >= 0);

    if (auto_more && !player_ptr->now_damaged) {
        show_more = is_msg_window_flowed();
    }

    if (skip_more) {
        show_more = false;
    }

    player_ptr->now_damaged = false;
    if (!player_ptr->playing || show_more) {
        put_more_prompt(x);
        more_prompt_x = x;
        waiting_for_more = true;
        const auto finalizer = util::make_finalizer([] { waiting_for_more = false; });
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

    erase_msg_line();
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
 * "msg_erase()" function, since this may result in the loss
 * of information if the screen is cleared, or if anything is
 * displayed on the top line.
 *
 * Note that "msg_erase()" will clear the top line
 * even if no messages are pending.  This is probably a hack.
 * @todo ここのp_ptrを削除するのは破滅的に作業が増えるので保留
 */
void msg_print(std::string_view msg)
{
    const auto &world = AngbandWorld::get_instance();
    if (world.timewalk_m_idx) {
        return;
    }

    if (!msg_flag) {
        erase_msg_line();
        msg_head_pos = 0;
    }

    std::string msg_includes_turn;
    if (cheat_turn) {
        msg = msg_includes_turn = fmt::format("T:{} - {}", world.game_turn, msg);
    }

    // -続く- の待ちの最中に端末の大きさが変わることがあるため、分割する幅は待った後に取り直す
    const auto get_split_width = [] {
        const auto &[wid, hgt] = term_get_size();
        return wid - 8;
    };

    if ((msg_head_pos > 0) && ((msg_head_pos + std::ssize(msg)) > get_split_width())) {
        msg_flush(p_ptr, msg_head_pos);
        msg_flag = false;
        msg_head_pos = 0;
    }

    if (msg.size() > 1000) {
        return;
    }

    if (world.character_generated) {
        message_add(msg);
    }

    for (auto split_width = get_split_width(); std::ssize(msg) > split_width; split_width = get_split_width()) {
        auto split = split_length(msg, split_width);
        put_msg_line(0, msg.substr(0, split));
        msg_flush(p_ptr, split + 1);
        msg.remove_prefix(split);
    }

    put_msg_line(msg_head_pos, msg);
    RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::MESSAGE);
    window_stuff(p_ptr);

    msg_flag = true;
    msg_head_pos += msg.size() + _(0, 1);

    if (fresh_message) {
        term_fresh_force();
    }
}

void msg_erase()
{
    if (AngbandWorld::get_instance().timewalk_m_idx) {
        return;
    }

    if (!msg_flag) {
        erase_msg_line();
        msg_head_pos = 0;
    }

    if (msg_head_pos > 0) {
        msg_flush(p_ptr, msg_head_pos);
        msg_flag = false;
        msg_head_pos = 0;
    }
}

/*!
 * @brief -続く- を表示してキー入力を待っている最中か否かを返す
 * @return 待っている最中ならtrue
 * @details
 * 待っている間に端末の大きさが変わると、リサイズフックが再描画のために呼ぶ
 * handle_stuff() から msg_erase() を経て -続く- がもう一度表示され、入力待ちが
 * 入れ子になる。フックはこれを見て、再描画を -続く- が明けた後に延期する。
 */
bool is_waiting_for_more()
{
    return waiting_for_more;
}

/*!
 * @brief -続く- の待ちの最中に端末の大きさが変わった時、メッセージ行と -続く- を見える位置に表示し直す
 * @details
 * 縮めると、変わる前の幅で表示した -続く- が画面の外に出て、見えないまま入力を待つことになる。
 * -続く- は端末の幅に合わせて表示する位置が変わるため、前に表示した -続く- が残らないよう、
 * メッセージ行を消去してメッセージから描き直す。待っていなければ何もしない。
 */
void redisplay_more_prompt()
{
    if (!waiting_for_more) {
        return;
    }

    term_erase(0, 0);
    term_putstr(0, 0, -1, TERM_WHITE, msg_line);
    put_more_prompt(more_prompt_x);
}

void msg_format(const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    const auto buf = vformat(fmt, vp);
    va_end(vp);
    msg_print(buf);
}

/*!
 * @brief セーブファイルにメッセージ履歴を保存する
 */
void wr_message_history()
{
    auto num = message_num();
    if (compress_savefile && (num > 40)) {
        num = 40;
    }

    wr_s32b(num);
    for (auto i = 0; i < num; ++i) {
        const auto &[msg, repeat_count] = message_history[i];
        wr_string(*msg);
        wr_s16b(repeat_count);
    }
}

/*!
 * @brief セーブファイルからメッセージ履歴を読み込む
 */
void rd_message_history()
{
    message_history.clear();

    const auto message_hisotry_num = rd_s32b();
    for (auto i = 0; i < message_hisotry_num; i++) {
        auto msg = rd_string();
        const auto repeat_count = rd_s16b();
        if (message_history.size() < MESSAGE_MAX) {
            message_history.emplace_back(make_message(std::move(msg)), repeat_count);
        }
    }
}
