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
#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <range/v3/view.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <type_traits>
#include <utility>

namespace {

constexpr auto HEADLESS_TERM_PROTOCOL_VERSION = 1; //!< プロトコルの版。互換性を壊す変更で更新する
constexpr auto HEADLESS_TERM_ACCEPT_TIMEOUT_SECONDS = 30; //!< 最初のクライアント接続を待つ秒数
constexpr auto HEADLESS_TERM_KEY_QUEUE_SIZE = 1024; //!< 1回のリクエストで長いキー列を注入できるようにするためのキュー長
constexpr auto HEADLESS_TERM_KEYS_BUFFER_SIZE = 1024; //!< text_to_ascii()に渡す変換先バッファの大きさ
constexpr auto HEADLESS_TERM_DEFAULT_MESSAGE_COUNT = 20; //!< messagesリクエストの既定取得件数
constexpr auto HEADLESS_TERM_KEYS_BUFFER_FILLER = '\xff'; //!< text_to_ascii()が書いた終端を見分けるための番兵

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

    fmt::println(stderr, "headless-term: {}", str);
    std::fflush(stderr);
}

/*!
 * @brief ゲーム終了時にソケットを閉じる
 * @details
 * 終了理由のメッセージはquit()/core()が自らplog()へ渡すため、ここでは出力しない。
 * plog_auxもheadless_term_plog()であり、出力すると標準エラー出力へ二重に表示される。
 */
void headless_term_quit(std::string_view)
{
    headless_term_server.reset();
}

/*!
 * @brief リクエストの処理結果
 * @details
 * 終了要求をレスポンスのキーで表すと、make_ok_response()がボディを展開する都合上、
 * ボディ側に同名のキーが現れた時に意図せず終了してしまう。
 * レスポンスとは別のフラグで持つことで、両者の取り違えを型で防ぐ。
 */
struct RequestResult {
    RequestResult(nlohmann::json response, bool is_quit_requested = false)
        : response(std::move(response))
        , is_quit_requested(is_quit_requested)
    {
    }

    nlohmann::json response; //!< クライアントへ返すレスポンス
    bool is_quit_requested; //!< ゲームの終了を要求されたか否か
};

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
 * @brief リクエストから指定した型の値を取り出す
 * @param request リクエストのJSONオブジェクト
 * @param key 取り出す値のキー
 * @param default_value キーが存在しない場合に返す既定値
 * @return 取り出した値。値の型が期待する型と異なる場合はnullopt
 * @details
 * nlohmann::json::value()は型が異なると例外を投げる。これをそのまま送出すると
 * serve_request()がidを持たないエラーレスポンスに変換してしまい、
 * クライアントがレスポンスをリクエストと対応付けられなくなるため、ここで型を検証する。
 */
template <typename T>
tl::optional<T> find_request_value(const nlohmann::json &request, const char *key, T default_value)
{
    const auto it = request.find(key);
    if ((it == request.end()) || it->is_null()) {
        return default_value;
    }

    const auto is_expected_type = [&it] {
        if constexpr (std::is_same_v<T, bool>) {
            return it->is_boolean();
        } else if constexpr (std::is_integral_v<T>) {
            return it->is_number_integer();
        } else {
            return it->is_string();
        }
    }();

    if (!is_expected_type) {
        return tl::nullopt;
    }

    return it->template get<T>();
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
 * @brief キー列のマクロ表記をtext_to_ascii()へ渡して安全か検証する
 * @param keys 検証するキー列
 * @return 渡してはならない場合はその理由。問題が無い場合はnullopt
 * @details
 * text_to_ascii()はstring_viewの長さではなくNUL終端を頼りに走査するため、
 * 「\x」のように末尾でエスケープの引数が足りていないと文字列の外まで読み進めてしまう。
 * 同じ歩幅で先に走査し、そのような入力を変換前に弾く。
 */
tl::optional<std::string> find_invalid_key_notation(std::string_view keys)
{
    // text_to_ascii()はNULを終端とみなして走査を打ち切るため、
    // そのまま渡すとキー列が黙って切り詰められる
    if (keys.find('\0') != std::string_view::npos) {
        return "the key sequence contains a NUL character";
    }

    const std::string incomplete_escape = "the key sequence ends with an incomplete escape";
    for (size_t i = 0; i < keys.length();) {
        const auto ch = keys[i];
        if (ch == '^') {
            // 「^X」はXを1文字消費する
            if (i + 1 >= keys.length()) {
                return incomplete_escape;
            }

            i += 2;
            continue;
        }

        if (ch != '\\') {
            i++;
            continue;
        }

        if (i + 1 >= keys.length()) {
            return incomplete_escape;
        }

        // 「\[～]」はマクロトリガ表記だが、ヘッドレス端末はpref-*.prfを読まずマクロトリガを
        // 定義しないため常に無意味であり、変換先の残量を見ずに書き込むため受け付けない
        if (keys[i + 1] == '[') {
            return "the macro trigger notation is not supported";
        }

        // 「\xNN」「\0NN」～「\3NN」は続く2文字を消費する
        const auto has_two_digits = (keys[i + 1] == 'x') || ((keys[i + 1] >= '0') && (keys[i + 1] <= '3'));
        const auto consumed = has_two_digits ? 4 : 2;
        if (i + consumed > keys.length()) {
            return incomplete_escape;
        }

        i += consumed;
    }

    return tl::nullopt;
}

/*!
 * @brief キー列のマクロ表記を解釈する
 * @param keys 解釈するキー列 (「\e」「^X」等のマクロ表記を使用できる)
 * @return 解釈した結果のバイト列。解釈できない場合はエラーの内容
 * @details
 * text_to_ascii()は書き込んだ長さを返さずNUL終端の文字列を書くだけであるため、
 * 変換前にバッファを番兵で埋めておき、末尾側に残った番兵の直前を終端とみなす。
 * こうすると「^@」等が生成する埋め込みのNULを終端と区別して検出できる。
 */
tl::expected<std::string, std::string> decode_keys(const std::string &keys)
{
    if (const auto invalid = find_invalid_key_notation(keys); invalid) {
        return tl::make_unexpected(*invalid);
    }

    std::array<char, HEADLESS_TERM_KEYS_BUFFER_SIZE> buffer;
    buffer.fill(HEADLESS_TERM_KEYS_BUFFER_FILLER);
    text_to_ascii(buffer.data(), keys, buffer.size());

    const auto terminator = std::find_if(buffer.rbegin(), buffer.rend(), [](char ch) { return ch != HEADLESS_TERM_KEYS_BUFFER_FILLER; });
    std::string decoded(buffer.data(), std::distance(terminator, buffer.rend()) - 1);

    // text_to_ascii()は変換先が尽きると無言で打ち切るため、出力が上限に達していれば
    // 切り詰められたものとして扱う。一部だけ注入された状態で成功を返すと、
    // キー列の再生結果が黙って食い違ってしまう
    if (decoded.size() + 1 >= buffer.size()) {
        return tl::make_unexpected("the key sequence is too long");
    }

    // term_key_push()はNULを積めないため、黙って取りこぼすのではなくエラーとする
    if (decoded.find('\0') != std::string::npos) {
        return tl::make_unexpected("the key sequence contains a NUL character");
    }

    return decoded;
}

/*!
 * @brief 現在の端末のキューにあと何個キーを積めるか数える
 * @return 積めるキーの数
 * @details
 * term_key_push()は空きが無くなっても積むことを拒否せず、循環キューを一周して
 * 先に積んだキーを壊すため、積む前に空きを確かめる必要がある。
 */
int count_key_queue_room()
{
    const auto size = game_term->key_size;
    if (size == 0) {
        return 0;
    }

    const auto pending = (game_term->key_head + size - game_term->key_tail) % size;
    return size - 1 - pending;
}

/*!
 * @brief キー列を解釈して現在の端末のキューへ注入する
 * @param keys 注入するキー列 (「\e」「^X」等のマクロ表記を使用できる)
 * @return キューへ積んだキーの数。積めなかった場合はエラーの内容
 * @details
 * term_key_push()はキューの先頭へ挿入するため、正しい順序で消費させるには
 * 末尾から逆順に積む必要がある (main-gcu.cppのterm_string_push()と同じ理由)。
 * 逆順に積む都合上、途中で空きが尽きると失われるのはキー列の先頭側になる。
 * 意味の異なる操作を実行させないよう、全て積めない場合は1つも積まない。
 */
tl::expected<int, std::string> push_keys(const std::string &keys)
{
    const auto decoded = decode_keys(keys);
    if (!decoded) {
        return tl::make_unexpected(decoded.error());
    }

    if (std::cmp_greater(decoded->size(), count_key_queue_room())) {
        return tl::make_unexpected("the key queue does not have enough room");
    }

    for (const auto key : *decoded | ranges::views::reverse) {
        (void)term_key_push(static_cast<unsigned char>(key));
    }

    return static_cast<int>(decoded->size());
}

/*!
 * @brief screenリクエストを処理する
 * @param id リクエストのid
 * @param request リクエストのJSONオブジェクト
 * @return レスポンスのJSONオブジェクト
 */
nlohmann::json handle_screen_request(const nlohmann::json &id, const nlohmann::json &request)
{
    const auto index = find_request_value(request, "term", 0);
    if (!index) {
        return make_error_response(id, "\"term\" must be an integer");
    }

    if ((*index < 0) || (*index >= headless_term_count)) {
        return make_error_response(id, "the term index is out of range");
    }

    const auto with_attrs = find_request_value(request, "attrs", true);
    if (!with_attrs) {
        return make_error_response(id, "\"attrs\" must be a boolean");
    }

    auto response = make_ok_response(id, make_headless_term_screen_json(headless_terms[*index], *with_attrs));
    response["term"] = *index;
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
 * @return リクエストの処理結果
 */
RequestResult dispatch_request(const nlohmann::json &request)
{
    if (!request.is_object()) {
        return make_error_response(nullptr, "the request must be a JSON object");
    }

    const auto id = request.value("id", nlohmann::json());
    const auto op = find_request_value(request, "op", std::string());
    if (!op) {
        return make_error_response(id, "\"op\" must be a string");
    }

    if (*op == "info") {
        return make_ok_response(id, make_info_response());
    }

    if (*op == "screen") {
        return handle_screen_request(id, request);
    }

    if (*op == "keys") {
        const auto keys = find_request_value(request, "keys", std::string());
        if (!keys) {
            return make_error_response(id, "\"keys\" must be a string");
        }

        if (keys->empty()) {
            return make_error_response(id, "\"keys\" must be a non-empty string");
        }

        const auto pushed = push_keys(*keys);
        if (!pushed) {
            return make_error_response(id, pushed.error());
        }

        return make_ok_response(id, { { "pushed", *pushed } });
    }

    if (*op == "state") {
        return handle_state_request(id);
    }

    if (*op == "messages") {
        const auto count = find_request_value(request, "count", HEADLESS_TERM_DEFAULT_MESSAGE_COUNT);
        if (!count) {
            return make_error_response(id, "\"count\" must be an integer");
        }

        return make_ok_response(id, { { "messages", make_messages_response(*count) } });
    }

    if (*op == "quit") {
        return RequestResult(make_ok_response(id, { { "quitting", true } }), true);
    }

    return make_error_response(id, "unknown op: " + *op);
}

/*!
 * @brief 接続済みのクライアントからリクエストを1件受信して処理する
 * @details
 * リクエストが1行分揃うまで待機する。クライアントが切断された場合と受信が期限切れになった場合は
 * 何もせずに戻り、呼び出し側のループが次のクライアントの接続を待ち直す。
 * 不正なJSONや内部状態の構築失敗でゲームを巻き込んで落とさないよう、
 * 例外は全てここで捕捉してエラーレスポンスに変換する。
 * 終了するか否かはレスポンスの内容ではなくRequestResultのフラグで判断する。
 */
void serve_request()
{
    const auto line = headless_term_server->receive_line();
    if (!line) {
        return;
    }

    RequestResult result{ nlohmann::json() };
    try {
        result = dispatch_request(nlohmann::json::parse(*line));
    } catch (const std::exception &e) {
        result = make_error_response(nullptr, e.what());
    }

    // 送信に失敗した場合はサーバ側がクライアントを切断済みで、呼び出し側のループが次の接続を待ち直す
    (void)headless_term_server->send_line(result.response.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));

    // レスポンスが届いたか否かに関わらず終了する。送信の失敗で終了要求を握り潰すと、
    // クライアントが去った後もプロセスが次の接続を待ち続けて残る
    if (result.is_quit_requested) {
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
        // 描画が確定していない画面を観測させないため、入力待ち以外ではリクエストを処理しない。
        // ここで処理しなくても、次の入力待ちで必ず処理される
        return 0;
    }

    while (game_term->key_head == game_term->key_tail) {
        if (!headless_term_server->ensure_client()) {
            quit("failed to accept a client connection");
            return 0;
        }

        serve_request();
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
