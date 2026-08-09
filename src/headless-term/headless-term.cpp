/*!
 * @file headless-term.cpp
 * @brief 実描画・実入力デバイスを持たない端末フロントエンドの実装
 * @details
 * 画面内容の読み出しとキー入力の注入を、ループバックTCP上の
 * 1行1JSONのリクエスト/レスポンスとして外部に公開する。
 *
 * 描画フックは何も行わない。z-termが要求バッファ(term_type::scr)を
 * 常に最新に保つため、描画の実体が無くても画面内容は完全に取得できる。
 *
 * ゲームがキー入力待ちに入った時 (TERM_XTRA_EVENTのwait=true) にのみ
 * リクエストを処理する。これにより、クライアントが観測する画面は
 * 必ず描画が確定した状態のものになる。
 */

#include "headless-term/headless-term.h"
#include "bot/bot-json-output.h"
#include "game-option/runtime-arguments.h"
#include "headless-term/headless-term-screen.h"
#include "headless-term/headless-term-server.h"
#include "io/macro-configurations-store.h"
#include "locale/character-encoding.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/term-color-types.h"
#include "term/z-term.h"
#include "term/z-util.h"
#include "util/int-char-converter.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tl/optional.hpp>

namespace {

constexpr auto HEADLESS_TERM_PROTOCOL_VERSION = 1; //!< プロトコルの版。互換性を壊す変更で更新する
constexpr auto HEADLESS_TERM_ACCEPT_TIMEOUT_SECONDS = 30; //!< 最初のクライアント接続を待つ秒数
constexpr auto HEADLESS_TERM_KEY_QUEUE_SIZE = 1024; //!< 1回のリクエストで長いキー列を注入できるようにするためのキュー長
constexpr auto HEADLESS_TERM_KEYS_BUFFER_SIZE = 1024; //!< text_to_ascii()に渡す変換先バッファの大きさ
constexpr auto HEADLESS_TERM_DEFAULT_MESSAGE_COUNT = 20; //!< messagesリクエストの既定取得件数

std::array<term_type, MAX_TERM_DATA> headless_terms;
int headless_term_count = 0;
std::unique_ptr<HeadlessTermServer> headless_term_server;

/*!
 * @brief 文字列描画要求を無視する
 * @return 常に0 (成功)
 * @details 画面内容はz-termの要求バッファから直接読み出すため、描画の実体は不要である。
 */
errr headless_term_text(TERM_LEN, TERM_LEN, int, TERM_COLOR, concptr)
{
    return 0;
}

/*!
 * @brief 消去要求を無視する
 * @return 常に0 (成功)
 */
errr headless_term_wipe(TERM_LEN, TERM_LEN, int)
{
    return 0;
}

/*!
 * @brief カーソル移動要求を無視する
 * @return 常に0 (成功)
 * @details カーソル位置はterm_win::cx/cyから読み出すため、移動の実体は不要である。
 */
errr headless_term_curs(TERM_LEN, TERM_LEN)
{
    return 0;
}

/*!
 * @brief 標準エラー出力へ診断メッセージを出力する
 * @param str 出力するメッセージ
 * @details
 * ヘッドレス運用ではメッセージボックスも端末画面も観測できないため、
 * plog()/quit()/core()の出力先を標準エラー出力へ差し替える。
 */
void headless_term_plog(std::string_view str)
{
    if (str.empty()) {
        return;
    }

    std::fprintf(stderr, "headless-term: %.*s\n", static_cast<int>(str.size()), str.data());
    std::fflush(stderr);
}

/*!
 * @brief ゲーム終了時にメッセージを出力してソケットを閉じる
 * @param str 終了理由のメッセージ
 */
void headless_term_quit(std::string_view str)
{
    headless_term_plog(str);
    headless_term_server.reset();
}

/*!
 * @brief エラーを表すレスポンスを生成する
 * @param id リクエストのid (無い場合はnull)
 * @param message エラーの内容
 * @return レスポンスのJSONオブジェクト
 */
nlohmann::json make_error_response(const nlohmann::json &id, std::string_view message)
{
    return {
        { "id", id },
        { "ok", false },
        { "error", std::string(message) },
    };
}

/*!
 * @brief 成功を表すレスポンスを生成する
 * @param id リクエストのid (無い場合はnull)
 * @param body レスポンスの中身
 * @return レスポンスのJSONオブジェクト
 */
nlohmann::json make_ok_response(const nlohmann::json &id, nlohmann::json body = nlohmann::json::object())
{
    body["id"] = id;
    body["ok"] = true;
    return body;
}

/*!
 * @brief infoリクエストに対する端末情報とビルド情報を生成する
 * @return レスポンスのJSONオブジェクト
 */
nlohmann::json make_info_response()
{
    auto terms = nlohmann::json::array();
    for (auto i = 0; i < headless_term_count; i++) {
        const auto &t = headless_terms[i];
        terms.push_back({
            { "index", i },
            { "width", static_cast<int>(t.wid) },
            { "height", static_cast<int>(t.hgt) },
        });
    }

    return {
        { "protocol_version", HEADLESS_TERM_PROTOCOL_VERSION },
        { "system", std::string(ANGBAND_SYS) },
        { "version", to_json_utf8(AngbandSystem::get_instance().build_version_expression(VersionExpression::FULL)) },
#ifdef JP
        { "japanese", true },
#else
        { "japanese", false },
#endif
        { "terms", terms },
    };
}

/*!
 * @brief 直近のメッセージ履歴を新しい順ではなく古い順に並べて生成する
 * @param count 取得する件数
 * @return メッセージ文字列のJSON配列
 */
nlohmann::json make_messages_response(int count)
{
    auto messages = nlohmann::json::array();
    const auto available = static_cast<int>(message_num());
    for (auto age = std::clamp(count, 0, available); age-- > 0;) {
        messages.push_back(to_json_utf8(*message_str(age)));
    }

    return messages;
}

/*!
 * @brief キー列を解釈して現在の端末のキューへ注入する
 * @param keys 注入するキー列 (「\e」「^X」等のマクロ表記を使用できる)
 * @return 実際にキューへ積んだキーの数。変換が切り詰められた場合はnullopt
 * @details
 * term_key_push()はキューの先頭へ挿入するため、正しい順序で消費させるには
 * 末尾から逆順に積む必要がある (main-gcu.cppのterm_string_push()と同じ理由)。
 */
tl::optional<int> push_keys(const std::string &keys)
{
    std::array<char, HEADLESS_TERM_KEYS_BUFFER_SIZE> buffer{};
    text_to_ascii(buffer.data(), keys, buffer.size());
    const std::string decoded(buffer.data());

    // text_to_ascii()は戻り値を持たず変換先が尽きると無言で打ち切るため、
    // 出力が上限に達していれば切り詰められたものとして扱う。
    // 一部だけ注入された状態で成功を返すと、キー列の再生結果が黙って食い違ってしまう
    if (decoded.size() + 1 >= buffer.size()) {
        return tl::nullopt;
    }

    auto pushed = 0;
    for (auto it = decoded.rbegin(); it != decoded.rend(); ++it) {
        if (term_key_push(static_cast<unsigned char>(*it)) >= 0) {
            pushed++;
        }
    }

    return pushed;
}

/*!
 * @brief screenリクエストを処理する
 * @param id リクエストのid
 * @param request リクエストのJSONオブジェクト
 * @return レスポンスのJSONオブジェクト
 */
nlohmann::json handle_screen_request(const nlohmann::json &id, const nlohmann::json &request)
{
    const auto index = request.value("term", 0);
    if ((index < 0) || (index >= headless_term_count)) {
        return make_error_response(id, "the term index is out of range");
    }

    auto response = make_ok_response(id, make_headless_term_screen_json(headless_terms[index], request.value("attrs", true)));
    response["term"] = index;
    return response;
}

/*!
 * @brief stateリクエストを処理する
 * @param id リクエストのid
 * @return レスポンスのJSONオブジェクト
 * @details キャラクターが生成される前は内部状態を構築できないためエラーを返す。
 */
nlohmann::json handle_state_request(const nlohmann::json &id)
{
    const auto is_ready = AngbandWorld::get_instance().character_generated && (p_ptr != nullptr) && (p_ptr->current_floor_ptr != nullptr);
    if (!is_ready) {
        return make_error_response(id, "the game state is not ready yet");
    }

    return make_ok_response(id, make_bot_json_snapshot(p_ptr));
}

/*!
 * @brief リクエストのJSONオブジェクトを解釈して処理する
 * @param request リクエストのJSONオブジェクト
 * @return レスポンスのJSONオブジェクト
 */
nlohmann::json dispatch_request(const nlohmann::json &request)
{
    if (!request.is_object()) {
        return make_error_response(nullptr, "the request must be a JSON object");
    }

    const auto id = request.value("id", nlohmann::json());
    const auto op = request.value("op", std::string());
    if (op == "info") {
        return make_ok_response(id, make_info_response());
    }

    if (op == "screen") {
        return handle_screen_request(id, request);
    }

    if (op == "keys") {
        const auto keys = request.value("keys", std::string());
        if (keys.empty()) {
            return make_error_response(id, "\"keys\" must be a non-empty string");
        }

        const auto pushed = push_keys(keys);
        if (!pushed) {
            return make_error_response(id, "the key sequence is too long");
        }

        return make_ok_response(id, { { "pushed", *pushed } });
    }

    if (op == "state") {
        return handle_state_request(id);
    }

    if (op == "messages") {
        return make_ok_response(id, { { "messages", make_messages_response(request.value("count", HEADLESS_TERM_DEFAULT_MESSAGE_COUNT)) } });
    }

    if (op == "quit") {
        return make_ok_response(id, { { "quitting", true } });
    }

    return make_error_response(id, "unknown op: " + op);
}

/*!
 * @brief 接続済みのクライアントからリクエストを1件受信して処理する
 * @param wait リクエストが1行分揃うまで待機するか否か
 * @details
 * リクエストが揃わなかった場合と送信に失敗した場合は何もせずに戻る。
 * 接続が切れていれば呼び出し側のループが次のクライアントの接続を待ち直す。
 * 不正なJSONや内部状態の構築失敗でゲームを巻き込んで落とさないよう、
 * 例外は全てここで捕捉してエラーレスポンスに変換する。
 */
void serve_pending_request(bool wait)
{
    const auto line = headless_term_server->receive_line(wait);
    if (!line) {
        return;
    }

    nlohmann::json response;
    try {
        response = dispatch_request(nlohmann::json::parse(*line));
    } catch (const std::exception &e) {
        response = make_error_response(nullptr, e.what());
    }

    const auto is_sent = headless_term_server->send_line(response.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
    if (is_sent && response.value("quitting", false)) {
        quit("");
    }
}

/*!
 * @brief キー入力イベントを処理する
 * @param wait キーが得られるまで待機するか否か
 * @return 常に0 (成功)
 * @details
 * 待機する場合はキューにキーが積まれるまでリクエストを処理し続ける。
 * この待機点がクライアントから見た唯一の同期点であり、
 * ここで返す画面は必ず描画が確定した状態のものになる。
 *
 * quit()/core()の後始末でサーバを破棄した後も、緊急セーブ等が入力待ちに入り得る。
 * その場合は待機せずにESCを積み、終了処理をそのまま進ませる。
 */
errr headless_term_process_events(bool wait)
{
    if (!headless_term_server) {
        // 応答する相手が居ないため、キーを積まないとterm_inkey()が無限に待ち続ける
        if (wait) {
            term_key_push(ESCAPE);
        }

        return 0;
    }

    if (!wait) {
        // 接続待ちでも受信待ちでもブロックしてはならないため、既に届いているものだけを処理する
        serve_pending_request(false);
        return 0;
    }

    while (game_term->key_head == game_term->key_tail) {
        if (!headless_term_server->ensure_client()) {
            quit("failed to accept a client connection");
            return 0;
        }

        serve_pending_request(true);
    }

    return 0;
}

/*!
 * @brief 拡張機能の要求を処理する
 * @param n 要求の種類
 * @param v 要求に伴う値
 * @return 常に0 (成功)
 * @details
 * 実際に処理する必要があるのはキー入力イベントだけである。
 * 画面消去や描画反映は要求バッファをそのまま読み出すため何もする必要がなく、
 * 音や演出は再生する相手が居ないため、いずれも成功として無視する。
 * TERM_XTRA_DELAYも同様に、演出を見せる相手が居ないため待たずに次の処理へ進む。
 */
errr headless_term_xtra(int n, int v)
{
    return (n == TERM_XTRA_EVENT) ? headless_term_process_events(v != 0) : 0;
}

/*!
 * @brief ヘッドレス端末を1つ初期化してangband_termsへ登録する
 * @param index angband_terms上の添字
 * @details
 * 画面サイズは最小構成の80x24で固定する。この値はMAIN_TERM_MIN_COLS/ROWSと等しく、
 * TermCenteredOffsetSetterによる中央寄せのオフセットが常に0になるため、
 * 要求バッファの座標がそのままゲーム側の描画座標と一致する。
 */
void term_data_init_headless(int index)
{
    auto *t = &headless_terms[index];
    term_init(t, TERM_DEFAULT_COLS, TERM_DEFAULT_ROWS, HEADLESS_TERM_KEY_QUEUE_SIZE);

    t->soft_cursor = true;
    t->never_bored = true;
    t->attr_blank = TERM_WHITE;
    t->char_blank = ' ';

    t->text_hook = headless_term_text;
    t->wipe_hook = headless_term_wipe;
    t->curs_hook = headless_term_curs;
    t->bigcurs_hook = headless_term_curs;
    t->xtra_hook = headless_term_xtra;

    term_activate(t);
    angband_terms[index] = t;
}

}

/*!
 * @brief ヘッドレス端末フロントエンドを初期化する
 * @return 初期化に成功した場合0、失敗した場合-1
 * @details
 * 待ち受けを開始した後、最初のクライアントが接続するまでブロックする。
 * これによりクライアントが接続した時点からゲームが始まり、
 * 起動直後のキー入力待ちも取りこぼさずに処理できる。
 */
errr init_headless_term()
{
    if (!arg_headless_port) {
        plog("The headless terminal requires the --headless-port=<port> option.");
        return -1;
    }

    // サーバの診断メッセージもplog()経由で標準エラー出力へ出すため、待ち受け開始より先に差し替える
    plog_aux = headless_term_plog;
    quit_aux = headless_term_quit;
    core_aux = headless_term_quit;

    auto server = std::make_unique<HeadlessTermServer>(*arg_headless_port);
    if (!server->listen_on_loopback() || !server->ensure_client(HEADLESS_TERM_ACCEPT_TIMEOUT_SECONDS)) {
        return -1;
    }

    headless_term_server = std::move(server);
    // 有効範囲はコマンドライン引数の解釈時に保証されている
    headless_term_count = arg_headless_term_count;
    // メイン端末を最後に有効化するため、添字の大きい方から初期化する
    for (auto i = headless_term_count; i-- > 0;) {
        term_data_init_headless(i);
    }

    term_screen = &headless_terms[0];
    return 0;
}
