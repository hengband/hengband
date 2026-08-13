/*!
 * @file bot-control-server.cpp
 * @brief 外部プログラムからゲームを操作する制御サーバの実装
 * @details
 * 画面内容の読み出しとキー入力の注入を、ループバックTCP上の
 * 1行1JSONのリクエスト/レスポンスとして外部に公開する。
 *
 * フロントエンドの種類には依存しない。画面内容はz-termが全フロントエンド共通で
 * 保持している要求バッファ(term_type::scr)から読み出すため、
 * GUIを表示したまま、人間の操作と併用できる。
 *
 * リクエストを処理するのは、ゲームがキー入力待ちに入った時だけである
 * (term_inkey_wait_hookが呼ばれる時)。これにより、クライアントが観測する画面は
 * 必ず描画が確定した状態のものになる。その代わり、長い処理の最中は応答が待たされる。
 */

#include "bot/bot-control-server.h"
#include "bot/bot-json-output.h"
#include "bot/bot-report.h"
#include "bot/bot-screen.h"
#include "bot/bot-socket-server.h"
#include "game-option/runtime-arguments.h"
#include "io/macro-configurations-store.h"
#include "system/angband-system.h"
#include "system/angband-version.h"
#include "system/player-type-definition.h"
#include "system/system-variables.h"
#include "term/gameterm.h"
#include "term/z-term.h"
#include "term/z-util.h"
#include "world/world.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <tl/expected.hpp>
#include <tl/optional.hpp>
#include <type_traits>
#include <utility>

namespace {

constexpr auto BOT_CONTROL_PROTOCOL_VERSION = 1; //!< プロトコルの版。互換性を壊す変更で更新する
constexpr auto BOT_CONTROL_KEYS_BUFFER_SIZE = 1024; //!< text_to_ascii()に渡す変換先バッファの大きさ
constexpr auto BOT_CONTROL_DEFAULT_MESSAGE_COUNT = 20; //!< messagesリクエストの既定取得件数
constexpr auto BOT_CONTROL_KEYS_BUFFER_FILLER = '\xff'; //!< text_to_ascii()が書いた終端を見分けるための番兵

/*!
 * @brief 1回の入力待ちでソケットを監視する長さ
 * @details
 * この間フロントエンドのイベントは取り出されないため、長くすると手元の操作が鈍る。
 * 逆に短くするとソケットを監視する回数が増える。人間が遅れを感じない範囲で長めに取る。
 */
constexpr auto BOT_CONTROL_POLL_INTERVAL = std::chrono::milliseconds(15);

std::unique_ptr<BotSocketServer> bot_control_server;

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
 * @brief 有効な端末か調べる
 * @param index angband_terms上の添字
 * @return 有効な端末である場合TRUE
 * @details
 * 生成されていない副端末の添字を指定された場合に弾くためのもの。
 * 画面バッファ(term_type::scr)はterm_init()で確保されterm_nuke()で解放されるため、
 * 生成前・破棄後の端末を読ませないようバッファの有無も併せて見る。
 */
bool is_valid_term_index(int index)
{
    if ((index < 0) || std::cmp_greater_equal(index, angband_terms.size())) {
        return false;
    }

    const auto *t = angband_terms[index];
    return (t != nullptr) && (t->scr != nullptr);
}

/*!
 * @brief infoリクエストに対する端末情報とビルド情報を生成する
 * @return レスポンスのJSONオブジェクト
 * @details
 * 端末の数と大きさはフロントエンドが決めるため、クライアントは画面を読む前に
 * ここで得た大きさを見ること。80x24とは限らない。
 * 列挙する端末はscreenリクエストと同じ判定で選び、ここに挙げた端末を
 * screenが拒む食い違いが起きないようにする。
 */
nlohmann::json make_info_response()
{
    auto terms = nlohmann::json::array();
    for (auto i = 0; std::cmp_less(i, angband_terms.size()); i++) {
        if (!is_valid_term_index(i)) {
            continue;
        }

        const auto *t = angband_terms[i];
        terms.push_back({
            { "index", i },
            { "width", static_cast<int>(t->wid) },
            { "height", static_cast<int>(t->hgt) },
        });
    }

    return {
        { "protocol_version", BOT_CONTROL_PROTOCOL_VERSION },
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
 * @brief キー列のマクロ表記をtext_to_ascii()へ渡して安全か検証する
 * @param keys 検証するキー列
 * @return 渡してはならない場合はその理由。問題が無い場合はnullopt
 * @details
 * text_to_ascii()はstring_viewの長さではなくNUL終端を頼りに走査するため、
 * 「\x」のように末尾でエスケープの引数が足りていないと文字列の外まで読み進めてしまう。
 * 同じ歩幅で先に走査し、そのような入力を変換前に弾く。
 * 以下の歩幅はio/macro-configurations-store.cppのtext_to_ascii()の分岐と対になっている。
 * 向こうを変更する場合はこちらも併せて見直すこと。
 */
tl::optional<std::string> find_invalid_key_notation(std::string_view keys)
{
    // text_to_ascii()はNULを終端とみなして走査を打ち切るため、
    // そのまま渡すとキー列が黙って切り詰められる
    if (keys.find('\0') != std::string_view::npos) {
        return "the key sequence contains a NUL character";
    }

    constexpr auto incomplete_escape = "the key sequence ends with an incomplete escape";
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

        // 「\[～]」はマクロトリガ表記だが、変換先の残量を見ずに書き込むため受け付けない
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
 * text_to_ascii() (io/macro-configurations-store.cpp) は書き込んだ長さを返さず
 * NUL終端の文字列を書くだけであるため、変換前にバッファを番兵で埋めておき、
 * 末尾側に残った番兵の直前を終端とみなす。
 * こうすると「^@」等が生成する埋め込みのNULを終端と区別して検出できる。
 */
tl::expected<std::string, std::string> decode_keys(const std::string &keys)
{
    if (const auto invalid = find_invalid_key_notation(keys); invalid) {
        return tl::make_unexpected(*invalid);
    }

    std::array<char, BOT_CONTROL_KEYS_BUFFER_SIZE> buffer;
    buffer.fill(BOT_CONTROL_KEYS_BUFFER_FILLER);
    text_to_ascii(buffer.data(), keys, buffer.size());

    const auto terminator = std::find_if(buffer.rbegin(), buffer.rend(), [](char ch) { return ch != BOT_CONTROL_KEYS_BUFFER_FILLER; });
    if (terminator == buffer.rend()) {
        // 番兵が1つも上書きされていない場合。text_to_ascii()は必ず終端NULを書くため
        // 現在の実装では起こらないが、長さが負値になる経路を残さないよう弾いておく
        return tl::make_unexpected("failed to decode the key sequence");
    }

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
 * @brief キー列を解釈して現在の端末のキューへ注入する
 * @param keys 注入するキー列 (「\e」「^X」等のマクロ表記を使用できる)
 * @return キューへ積んだキーの数。積めなかった場合はエラーの内容
 */
tl::expected<int, std::string> push_keys(const std::string &keys)
{
    const auto decoded = decode_keys(keys);
    if (!decoded) {
        return tl::make_unexpected(decoded.error());
    }

    if (term_keys_push(*decoded) != 0) {
        return tl::make_unexpected("the key queue does not have enough room");
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

    if (!is_valid_term_index(*index)) {
        return make_error_response(id, "the term index is out of range");
    }

    const auto with_attrs = find_request_value(request, "attrs", true);
    if (!with_attrs) {
        return make_error_response(id, "\"attrs\" must be a boolean");
    }

    auto response = make_ok_response(id, make_bot_screen_json(*angband_terms[*index], *with_attrs));
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
        const auto count = find_request_value(request, "count", BOT_CONTROL_DEFAULT_MESSAGE_COUNT);
        if (!count) {
            return make_error_response(id, "\"count\" must be an integer");
        }

        return make_ok_response(id, { { "messages", make_message_history_json(*count) } });
    }

    if (*op == "quit") {
        return RequestResult(make_ok_response(id, { { "quitting", true } }), true);
    }

    return make_error_response(id, "unknown op: " + *op);
}

/*!
 * @brief 接続済みのクライアントからリクエストを1件受信して処理する
 * @param deadline 受信と送信の待機に用いる期限
 * @details
 * クライアントが切断された場合と、期限内に1行が揃わなかった場合は何もせずに戻る。
 * 受信の途中経過はサーバが保持しているため、続きは次の入力待ちで組み立てられる。
 * 不正なJSONや内部状態の構築失敗でゲームを巻き込んで落とさないよう、
 * 例外は全てここで捕捉してエラーレスポンスに変換する。
 * 終了するか否かはレスポンスの内容ではなくRequestResultのフラグで判断する。
 */
void serve_request(const SocketWaitDeadline &deadline)
{
    const auto line = bot_control_server->receive_line(deadline);
    if (!line) {
        return;
    }

    RequestResult result{ nlohmann::json() };
    try {
        result = dispatch_request(nlohmann::json::parse(*line));
    } catch (const std::exception &e) {
        result = make_error_response(nullptr, e.what());
    }

    // 期限内に送り切れなかった分はサーバが保持し、次の入力待ちで続きを送る
    (void)bot_control_server->send_line(result.response.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace), deadline);

    // レスポンスが届いたか否かに関わらず終了する。送信の失敗で終了要求を握り潰すと、
    // クライアントが去った後もプロセスが残る
    if (result.is_quit_requested) {
        bot_control_server->flush_response_until_timeout();
        quit("");
    }
}

/*!
 * @brief キー入力待ちの間に呼ばれ、届いているリクエストを処理する
 * @details
 * ゲームの進行を止めないよう、接続・受信・送信の全てを短い期限で打ち切って必ず戻る。
 * 何も届いていなければ何もしない。呼び出し側のterm_inkey()が
 * フロントエンドのイベント処理と本関数を交互に繰り返す。
 * 期限内に処理し切れなかった受信と送信はサーバが途中の状態を保持しており、
 * 次の呼び出しで続きから再開する。
 */
void serve_pending_requests()
{
    if (!bot_control_server) {
        // サーバが失われた後はフロントエンド本来のブロッキング待機に戻す
        term_inkey_wait_hook = nullptr;
        return;
    }

    // 接続・受信・送信で期限を共有し、1回の入力待ちで止まる時間がBOT_CONTROL_POLL_INTERVALを超えないようにする
    const auto deadline = make_deadline(BOT_CONTROL_POLL_INTERVAL);

    // 送り切れていないレスポンスがあれば、次のリクエストを受ける前に送り切る
    if (!bot_control_server->flush_response(deadline)) {
        return;
    }

    if (!bot_control_server->accept_client(deadline)) {
        return;
    }

    if (!bot_control_server->wait_readable(deadline)) {
        return;
    }

    serve_request(deadline);
}

}

/*!
 * @brief 制御サーバを起動する
 * @details
 * --control-portが指定されていない場合は何もしない。
 * 待ち受けを開始するだけでクライアントの接続は待たないため、ゲームの開始は妨げない。
 * 明示的に指定されたポートで待ち受けられないまま起動しても意図した操作ができないため、
 * 失敗した場合は理由を通知して終了する。
 */
void init_bot_control_server()
{
    if (!arg_control_port) {
        return;
    }

    prepare_bot_report();
    auto server = std::make_unique<BotSocketServer>(*arg_control_port);
    if (!server->listen_on_loopback()) {
        quit("Unable to start the game control server!");
    }

    bot_control_server = std::move(server);
    term_inkey_wait_hook = serve_pending_requests;
}

/*!
 * @brief 制御サーバを停止する
 * @details 停止後の入力待ちはフロントエンド本来のものに戻る。
 */
void shutdown_bot_control_server()
{
    term_inkey_wait_hook = nullptr;
    bot_control_server.reset();
}
