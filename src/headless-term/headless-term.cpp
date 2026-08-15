/*!
 * @file headless-term.cpp
 * @brief 実描画・実入力デバイスを持たない端末フロントエンドの実装
 * @details
 * ウィンドウも端末も開かず、制御サーバ (--control-port) 経由の操作だけでゲームを進める。
 *
 * 描画フックは登録しない。z-termが要求バッファ(term_type::scr)を常に最新に保つため、
 * 描画の実体が無くても制御サーバは画面内容を完全に読み出せる。
 * term_init()が用意する何もしないフックをそのまま使う。
 *
 * リクエストの受け付けそのものは制御サーバが担う。本フロントエンドが行うのは、
 * 入力イベントを持たないことをterm_type::never_eventsで申告し、
 * ゲームの終了時にサーバを止めることだけである。
 */

#include "headless-term/headless-term.h"
#include "bot/bot-control-server.h"
#include "game-option/runtime-arguments.h"
#include "locale/character-encoding.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "term/z-term.h"
#include "term/z-util.h"
#include "util/int-char-converter.h"
#include <array>
#include <cstdio>
#include <fmt/format.h>
#include <string>
#include <string_view>

namespace {

/*!
 * @brief ヘッドレス端末のキーキューの大きさ
 * @details
 * 制御サーバが1リクエストで受け付けるキー列を1度に積み切れるようにする。
 * キューの空きは全体から1つ少ない (term_key_queue_room()) ため、上限より1つ大きく取る。
 */
constexpr auto HEADLESS_TERM_KEY_QUEUE_SIZE = BOT_CONTROL_MAX_KEYS + 1;

std::array<term_type, MAX_TERM_DATA> headless_terms;

//! init_headless_term()がquit_auxを上書きする前に設定されていた終了処理
void (*prev_quit_aux)(std::string_view) = nullptr;

/*!
 * @brief 拡張機能の要求を処理する
 * @param n 要求の種類
 * @param v 要求に伴う値
 * @return 保留中のイベントが無い場合1、それ以外は0 (成功)
 * @details
 * 実際に応答が要るのはキー入力イベントだけである。画面消去や描画反映は
 * 要求バッファをそのまま読み出すため何もする必要がなく、音や演出は再生する相手が居ない。
 * TERM_XTRA_DELAYも同様に、演出を見せる相手が居ないため待たずに次の処理へ進む。
 */
errr headless_term_xtra(int n, int v)
{
    if (n != TERM_XTRA_EVENT) {
        return 0;
    }

    if (v == 0) {
        // 取り出せるイベントを持たない。ここで0を返すと、保留中のイベントを取り切ろうとする
        // term_inkey()のループ (z-term.cppを参照) が終わらなくなる
        return 1;
    }

    // 制御サーバが動いている間、入力待ちはterm_inkey_wait_hookが担うためここへは来ない。
    // 来るのはサーバが失われた後 (quit()/core()の後始末で緊急セーブが入力を待つ等) であり、
    // 応答する相手が居ないためキーを積まないとterm_inkey()が無限に待ち続ける
    term_key_push(ESCAPE);
    return 0;
}

/*!
 * @brief 標準エラー出力へ診断メッセージを出力する
 * @param str 出力するメッセージ
 * @details
 * ヘッドレス運用ではメッセージボックスも端末画面も観測できないため、
 * plog()/quit()/core()の出力先を標準エラー出力へ差し替える。
 *
 * メッセージはゲーム内部の文字コード (日本語版ではEUC-JP) であり、そのまま書くと
 * UTF-8の端末やログでは読めない。標準エラー出力がヘッドレス運用で唯一の
 * 情報源になるため、UTF-8へ変換してから出力する。
 */
void headless_term_plog(std::string_view str)
{
    if (str.empty()) {
        return;
    }

    // 変換に失敗しても、読めないよりは生のまま出す方が手掛かりになる
    const auto converted = sys_to_utf8(str);
    fmt::println(stderr, "headless-term: {}", converted.value_or(std::string(str)));
    std::fflush(stderr);
}

/*!
 * @brief ゲーム終了時に制御サーバを停止し、上書き前の終了処理へ委譲する
 * @param str 終了理由のメッセージ
 * @details
 * 終了理由のメッセージはquit()/core()が自らplog()へ渡すため、ここでは出力しない。
 * plog_auxもheadless_term_plog()であり、出力すると標準エラー出力へ二重に表示される。
 *
 * 端末の破棄などプラットフォーム側が用意した後始末を飛ばさないよう、上書き前のフックへ委譲する。
 * Unixではmain.cppのquit_hook()がterm_nuke()を呼ぶ。term_nuke()はWindowsでは定義されない
 * (z-term.cppの#ifndef WINDOWS) ため、ここから直接呼ぶことはできない。
 */
void headless_term_quit(std::string_view str)
{
    shutdown_bot_control_server();

    if (prev_quit_aux != nullptr) {
        prev_quit_aux(str);
    }
}

/*!
 * @brief ヘッドレス端末を1つ初期化してangband_termsへ登録する
 * @param index angband_terms上の添字
 * @details
 * 画面サイズは最小構成の80x24で固定する。この値はMAIN_TERM_MIN_COLS/ROWSと等しく、
 * TermCenteredOffsetSetterによる中央寄せのオフセットが常に0になるため、
 * 要求バッファの座標がそのままゲーム側の描画座標と一致する。
 *
 * 描画フックは登録しない。term_init()が何もしないフックを用意するため、
 * 描画するものが無い端末では上書きする必要が無い。
 */
void term_data_init_headless(int index)
{
    auto *t = &headless_terms[index];
    term_init(t, TERM_DEFAULT_COLS, TERM_DEFAULT_ROWS, HEADLESS_TERM_KEY_QUEUE_SIZE);

    t->soft_cursor = true;
    t->never_bored = true;
    t->never_events = true;
    t->attr_blank = TERM_WHITE;
    t->char_blank = ' ';

    t->xtra_hook = headless_term_xtra;

    term_activate(t);
    angband_terms[index] = t;
}

}

/*!
 * @brief ヘッドレス端末フロントエンドを初期化する
 * @return 初期化に成功した場合0、失敗した場合-1
 * @details
 * 端末を用意するだけで、クライアントの接続は待たない。
 * ヘッドレス端末は取り出せるイベントを持たないため、最初のキー入力待ちが
 * そのまま接続待ちとなり、クライアントが操作を始めるまでゲームは進まない。
 */
errr init_headless_term()
{
    if (!arg_control_port) {
        headless_term_plog("The headless terminal requires the --control-port=<port> option.");
        return -1;
    }

    // ゲームの診断メッセージも標準エラー出力へ出す。制御サーバの診断は
    // report_bot_message() が同じ出力先へ別の接頭辞で出す
    plog_aux = headless_term_plog;
    prev_quit_aux = quit_aux;
    quit_aux = headless_term_quit;
    core_aux = headless_term_quit;

    // pref-gcu.prf・pref-win.prf等を読ませないための上書き。フロントエンド固有の
    // マクロトリガが定義されないため、同じキー列がプラットフォームを問わず同じ結果になる
    ANGBAND_SYS = "headless";

    // 有効範囲はコマンドライン引数の解釈時に保証されている
    // メイン端末を最後に有効化するため、添字の大きい方から初期化する
    for (auto i = arg_headless_term_count.value_or(1); i-- > 0;) {
        term_data_init_headless(i);
    }

    term_screen = &headless_terms[0];
    return 0;
}
